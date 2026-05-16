#include "battery.h"

#include <string.h>

#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

static const char *TAG = "bebopCORE_battery";

static bebopCORE_battery_config_t s_config;
static bool s_initialized = false;
static uint8_t s_percent = 100;
static bool s_charging = false;
static bool s_critical = false;
static adc_oneshot_unit_handle_t s_adc_handle;
static adc_channel_t s_adc_channel;

static uint8_t battery_percent_from_voltage(float voltage)
{
    if (voltage <= 3.20f)
    {
        return 5;
    }
    if (voltage <= 3.35f)
    {
        return 15;
    }
    if (voltage <= 3.55f)
    {
        return 30;
    }
    if (voltage <= 3.75f)
    {
        return 50;
    }
    if (voltage <= 3.95f)
    {
        return 75;
    }
    return 100;
}

void battery_init(const bebopCORE_battery_config_t *config)
{
    memset(&s_config, 0, sizeof(s_config));
    if (config != NULL)
    {
        s_config = *config;
    }

    s_percent = 100;
    s_charging = false;
    s_critical = false;

    if (!s_config.enabled)
    {
        ESP_LOGI(TAG, "Battery ADC disabled; using fallback level");
        return;
    }

    adc_unit_t unit_id = ADC_UNIT_1;
    ESP_ERROR_CHECK(adc_oneshot_io_to_channel(s_config.adc_gpio, &unit_id, &s_adc_channel));

    adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id = unit_id,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_cfg, &s_adc_handle));

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc_handle, s_adc_channel, &chan_cfg));
    s_initialized = true;

    if (s_config.charging_gpio >= 0)
    {
        gpio_config_t cgcfg = {
            .pin_bit_mask = (1ULL << s_config.charging_gpio),
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        gpio_config(&cgcfg);
        ESP_LOGI(TAG, "Charging detect GPIO %d", s_config.charging_gpio);
    }
}

void battery_update(void)
{
    if (!s_config.enabled || !s_initialized)
    {
        return;
    }

    int raw = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(s_adc_handle, s_adc_channel, &raw));

    float sensed_voltage = ((float)raw / 4095.0f) * 3.3f;
    float cell_voltage = (sensed_voltage * s_config.divider_ratio) + s_config.offset_volts;
    s_percent = battery_percent_from_voltage(cell_voltage);
    s_critical = (cell_voltage <= 3.15f);

    if (s_config.charging_gpio >= 0)
    {
        s_charging = (gpio_get_level((gpio_num_t)s_config.charging_gpio) == 0);
    }
}

uint8_t battery_get_percent(void)
{
    return s_percent;
}

bool battery_is_charging(void)
{
    return s_charging;
}

bool battery_is_critical(void)
{
    return s_critical;
}
