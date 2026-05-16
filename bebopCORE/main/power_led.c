#include "power_led.h"

#include "driver/gpio.h"
#include "esp_timer.h"
#include "sdkconfig.h"

#define BEBOPCORE_GPIO_POWER_LED ((gpio_num_t)CONFIG_BEBOPCORE_POWER_LED_GPIO)

static uint8_t s_battery_percent = 100;
static bool s_charging = false;
static bool s_led_on = false;
static int64_t s_last_toggle_us = 0;

static int64_t power_led_interval_us(uint8_t battery)
{
    // Red power LED blinks faster as battery gets lower.
    if (battery <= 10)
    {
        return 120000;
    }
    if (battery <= 25)
    {
        return 220000;
    }
    if (battery <= 50)
    {
        return 350000;
    }
    return 700000;
}

void power_led_init(void)
{
    if (BEBOPCORE_GPIO_POWER_LED < 0)
    {
        return;
    }

    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << BEBOPCORE_GPIO_POWER_LED),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    gpio_config(&cfg);
    gpio_set_level(BEBOPCORE_GPIO_POWER_LED, 0);
    s_last_toggle_us = esp_timer_get_time();
}

void power_led_set_battery(uint8_t battery_percent)
{
    s_battery_percent = battery_percent;
}

void power_led_set_charging(bool charging)
{
    s_charging = charging;
}

void power_led_tick(void)
{
    if (BEBOPCORE_GPIO_POWER_LED < 0)
    {
        return;
    }

    int64_t now = esp_timer_get_time();
    if (s_charging)
    {
        gpio_set_level(BEBOPCORE_GPIO_POWER_LED, 1);
        s_led_on = true;
        s_last_toggle_us = now;
        return;
    }

    int64_t interval = power_led_interval_us(s_battery_percent);

    if ((now - s_last_toggle_us) >= interval)
    {
        s_led_on = !s_led_on;
        gpio_set_level(BEBOPCORE_GPIO_POWER_LED, s_led_on ? 1 : 0);
        s_last_toggle_us = now;
    }
}
