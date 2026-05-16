#include <stdbool.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "sdkconfig.h"

#include "app_state.h"
#include "battery.h"
#include "bebopCORE_types.h"
#include "bt_manager.h"
#include "input.h"
#include "power_led.h"
#include "protocol_manager.h"
#include "status_leds.h"
#include "storage.h"

static const char *TAG = "bebopCORE_main";

static bebopCORE_input_backend_t input_backend_from_kconfig(void)
{
#if CONFIG_BEBOPCORE_INPUT_BACKEND_MATRIX
    return BEBOPCORE_INPUT_BACKEND_MATRIX;
#else
    return BEBOPCORE_INPUT_BACKEND_GPIO;
#endif
}

static bebopCORE_battery_config_t battery_config_from_kconfig(void)
{
#ifdef CONFIG_BEBOPCORE_BATTERY_ADC_GPIO
    const uint8_t adc_gpio = CONFIG_BEBOPCORE_BATTERY_ADC_GPIO;
#else
    const uint8_t adc_gpio = 35;
#endif

#ifdef CONFIG_BEBOPCORE_BATTERY_DIVIDER_RATIO_MILLI
    const float divider_ratio = ((float)CONFIG_BEBOPCORE_BATTERY_DIVIDER_RATIO_MILLI) / 1000.0f;
#else
    const float divider_ratio = 2.0f;
#endif

#ifdef CONFIG_BEBOPCORE_BATTERY_OFFSET_MV
    const float offset_volts = ((float)CONFIG_BEBOPCORE_BATTERY_OFFSET_MV) / 1000.0f;
#else
    const float offset_volts = 0.0f;
#endif

    const bebopCORE_battery_config_t config = {
        .enabled = CONFIG_BEBOPCORE_BATTERY_ADC_ENABLED,
        .adc_gpio = adc_gpio,
        .divider_ratio = divider_ratio,
        .offset_volts = offset_volts,
        .charging_gpio = CONFIG_BEBOPCORE_BATTERY_CHARGE_GPIO,
    };

    return config;
}

static void apply_system_buttons(void)
{
    static bool pairing_hold_consumed = false;
    static bool recovery_hold_consumed = false;

    bool shift = input_is_pressed(BEBOPCORE_BUTTON_SHIFT);
    bool protocol_pressed = input_is_pressed(BEBOPCORE_BUTTON_PROTOCOL);
    bool start_pressed = input_is_pressed(BEBOPCORE_BUTTON_START);

    if (input_was_pressed(BEBOPCORE_BUTTON_PROTOCOL))
    {
        if (shift)
        {
            protocol_manager_reset_pairing();
            bt_manager_clear_pairing();
            status_leds_set_pairing_pulse();
        }
        else
        {
            protocol_manager_next();
            bt_manager_switch_protocol(protocol_manager_get_active());
            status_leds_set_protocol_change_pulse();
        }
    }

    if (!(shift && protocol_pressed))
    {
        pairing_hold_consumed = false;
    }

    if (!(shift && start_pressed))
    {
        recovery_hold_consumed = false;
    }

    if (shift && protocol_pressed && !pairing_hold_consumed &&
        input_is_held(BEBOPCORE_BUTTON_PROTOCOL, CONFIG_BEBOPCORE_PROTOCOL_HOLD_PAIRING_MS))
    {
        protocol_manager_enter_pairing();
        status_leds_set_pairing_pulse();
        pairing_hold_consumed = true;
    }

    if (shift && start_pressed && !recovery_hold_consumed &&
        input_is_held(BEBOPCORE_BUTTON_START, CONFIG_BEBOPCORE_RECOVERY_HOLD_MS))
    {
        app_state_request_recovery();
        recovery_hold_consumed = true;
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting bebopCORE (single firmware, expansion-ready)");

    storage_init();
    app_state_init();

    const bebopCORE_input_config_t input_config = {
        .backend = input_backend_from_kconfig(),
        .active_low = CONFIG_BEBOPCORE_INPUT_ACTIVE_LOW,
    };

    const bebopCORE_battery_config_t battery_config = battery_config_from_kconfig();

    input_init(&input_config);
    battery_init(&battery_config);
    protocol_manager_init();
    status_leds_init();
    power_led_init();
    bt_manager_init(protocol_manager_get_active());

    while (true)
    {
        input_update();
        battery_update();
        const bebopCORE_input_state_t *state = input_get_state();
        bebopCORE_report_t report;

        bool shift = state->buttons[BEBOPCORE_BUTTON_SHIFT];
        app_state_set_shift_active(shift);

        apply_system_buttons();
        input_build_report(&report, protocol_manager_get_active(), shift);
        protocol_manager_process_report(&report, shift);

        /* Forward report to the active BT backend */
        bt_manager_send_report(&report);

        uint8_t battery = battery_get_percent();
        bool charging = battery_is_charging();
        app_state_set_battery(battery, charging);

        status_leds_set_protocol(protocol_manager_get_active());
        status_leds_set_connection(protocol_manager_get_connection_state());
        status_leds_set_battery(battery);
        status_leds_set_shift(shift);

        power_led_set_battery(battery);
        power_led_set_charging(charging);
        power_led_tick();
        status_leds_tick();

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
