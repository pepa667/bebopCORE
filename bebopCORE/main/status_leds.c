#include "status_leds.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "sdkconfig.h"

static const char *TAG = "bebopCORE_status_leds";

/* ---------- GPIO helpers -------------------------------------------------- */

#define GPIO_CONN ((gpio_num_t)CONFIG_BEBOPCORE_LED_CONN_GPIO)
#define GPIO_PROTO ((gpio_num_t)CONFIG_BEBOPCORE_LED_PROTO_GPIO)

static inline bool led_conn_enabled(void) { return GPIO_CONN >= 0; }
static inline bool led_proto_enabled(void) { return GPIO_PROTO >= 0; }

static void led_set(gpio_num_t pin, bool on)
{
    if (pin >= 0)
    {
        gpio_set_level(pin, on ? 1 : 0);
    }
}

static void led_init_gpio(gpio_num_t pin)
{
    if (pin < 0)
    {
        return;
    }
    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << pin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&cfg);
    gpio_set_level(pin, 0);
}

/* ---------- state --------------------------------------------------------- */

static bebopCORE_protocol_t s_protocol = BEBOPCORE_PROTOCOL_SWITCH;
static bebopCORE_connection_state_t s_connection = BEBOPCORE_CONN_IDLE;
static uint8_t s_battery = 100;
static bool s_shift = false;

/* protocol-change: blink N times then pause; N = protocol index + 1 */
static int s_proto_blinks_remaining = 0;
static bool s_proto_blink_on = false;
static int64_t s_proto_blink_next_us = 0;

/* pairing pulse: fast flash on connection LED */
static int64_t s_pairing_pulse_until_us = 0;

/* ---------- blink engine -------------------------------------------------- */

#define BLINK_HALF_PERIOD_US 150000 /* 150 ms per half = 300 ms per blink   */
#define BLINK_GAP_US 700000         /* 700 ms gap between burst sequences   */
#define PAIRING_HALF_US 80000       /* 80 ms per half (fast during pairing) */
#define SEARCHING_HALF_US 300000    /* 300 ms per half                      */

static void proto_blink_tick(int64_t now)
{
    if (!led_proto_enabled())
    {
        return;
    }

    if (s_proto_blinks_remaining > 0 && now >= s_proto_blink_next_us)
    {
        s_proto_blink_on = !s_proto_blink_on;
        led_set(GPIO_PROTO, s_proto_blink_on);

        if (!s_proto_blink_on)
        {
            s_proto_blinks_remaining--;
        }

        s_proto_blink_next_us = now + BLINK_HALF_PERIOD_US;
    }
    else if (s_proto_blinks_remaining == 0)
    {
        led_set(GPIO_PROTO, false);
    }
}

static void conn_led_tick(int64_t now)
{
    if (!led_conn_enabled())
    {
        return;
    }

    /* pairing pulse overrides everything */
    if (now < s_pairing_pulse_until_us)
    {
        static int64_t pulse_next = 0;
        static bool pulse_on = false;
        if (now >= pulse_next)
        {
            pulse_on = !pulse_on;
            led_set(GPIO_CONN, pulse_on);
            pulse_next = now + PAIRING_HALF_US;
        }
        return;
    }

    switch (s_connection)
    {
    case BEBOPCORE_CONN_IDLE:
        led_set(GPIO_CONN, false);
        break;

    case BEBOPCORE_CONN_SEARCHING:
    {
        static int64_t t = 0;
        static bool on = false;
        if (now >= t)
        {
            on = !on;
            led_set(GPIO_CONN, on);
            t = now + SEARCHING_HALF_US;
        }
        break;
    }

    case BEBOPCORE_CONN_PAIRING:
    {
        static int64_t t = 0;
        static bool on = false;
        if (now >= t)
        {
            on = !on;
            led_set(GPIO_CONN, on);
            t = now + PAIRING_HALF_US;
        }
        break;
    }

    case BEBOPCORE_CONN_CONNECTED:
        led_set(GPIO_CONN, true);
        break;
    }
}

/* ---------- public API ---------------------------------------------------- */

void status_leds_init(void)
{
    led_init_gpio(GPIO_CONN);
    led_init_gpio(GPIO_PROTO);
    ESP_LOGI(TAG, "Status LEDs initialized (conn=%d proto=%d)", GPIO_CONN, GPIO_PROTO);
}

void status_leds_set_protocol(bebopCORE_protocol_t protocol)
{
    s_protocol = protocol;
}

void status_leds_set_connection(bebopCORE_connection_state_t state)
{
    s_connection = state;
}

void status_leds_set_battery(uint8_t battery_percent)
{
    s_battery = battery_percent;
}

void status_leds_set_shift(bool enabled)
{
    s_shift = enabled;
}

void status_leds_set_protocol_change_pulse(void)
{
    /* Blink N+1 times to indicate active protocol (Switch=1, XInput=2 …) */
    s_proto_blinks_remaining = (int)s_protocol + 1;
    s_proto_blink_on = false;
    s_proto_blink_next_us = esp_timer_get_time();
}

void status_leds_set_pairing_pulse(void)
{
    s_pairing_pulse_until_us = esp_timer_get_time() + 500000;
}

void status_leds_tick(void)
{
    int64_t now = esp_timer_get_time();
    proto_blink_tick(now);
    conn_led_tick(now);

    /* Periodic log when no GPIO is configured */
    if (!led_conn_enabled() && !led_proto_enabled())
    {
        static uint32_t decimator = 0;
        decimator++;
        if (decimator % 200 == 0)
        {
            ESP_LOGI(TAG,
                     "proto=%d conn=%d batt=%u%% shift=%d",
                     (int)s_protocol, (int)s_connection, s_battery, s_shift);
        }
    }
}
