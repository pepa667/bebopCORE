#include "protocol_manager.h"

#include "esp_log.h"

#include "app_state.h"
#include "storage.h"

static const char *TAG = "bebopCORE_protocol";

static bebopCORE_protocol_t s_active_protocol = BEBOPCORE_PROTOCOL_SWITCH;
static bebopCORE_connection_state_t s_connection_state = BEBOPCORE_CONN_IDLE;
static uint32_t s_pairing_reset_counter = 0;

static const char *s_protocol_names[BEBOPCORE_PROTOCOL_COUNT] = {
    [BEBOPCORE_PROTOCOL_SWITCH] = "Switch",
    [BEBOPCORE_PROTOCOL_XINPUT] = "XInput",
    [BEBOPCORE_PROTOCOL_DINPUT] = "DInput",
    [BEBOPCORE_PROTOCOL_GENERIC] = "Generic",
};

static bool protocol_is_supported(bebopCORE_protocol_t protocol)
{
    return protocol == BEBOPCORE_PROTOCOL_SWITCH;
}

static bebopCORE_protocol_t normalize_protocol(bebopCORE_protocol_t protocol)
{
    if (protocol_is_supported(protocol))
    {
        return protocol;
    }

    ESP_LOGW(TAG, "Protocol %d not implemented yet, falling back to Switch", (int)protocol);
    return BEBOPCORE_PROTOCOL_SWITCH;
}

static bool report_has_activity(const bebopCORE_report_t *report)
{
    return report->button_a || report->button_b || report->button_x || report->button_y ||
           report->dpad_right || report->dpad_down || report->dpad_left || report->dpad_up ||
           report->trigger_l || report->trigger_zl || report->trigger_r || report->trigger_zr ||
           report->button_start || report->button_select || report->button_home || report->button_capture ||
           report->button_stick_left || report->button_stick_right ||
           report->lx != 0 || report->ly != 0 || report->rx != 0 || report->ry != 0 ||
           report->lt != 0 || report->rt != 0;
}

void protocol_manager_init(void)
{
    s_active_protocol = normalize_protocol(storage_load_active_protocol());
    s_pairing_reset_counter = storage_load_pairing_reset_counter();
    s_connection_state = BEBOPCORE_CONN_IDLE;
    storage_save_active_protocol(s_active_protocol);
    app_state_set_protocol(s_active_protocol);
    ESP_LOGI(TAG, "Protocol manager initialized in single-firmware mode (%s)", protocol_manager_get_active_name());
}

bool protocol_manager_next(void)
{
    bebopCORE_protocol_t next_protocol = s_active_protocol;

    do
    {
        next_protocol = (next_protocol + 1) % BEBOPCORE_PROTOCOL_COUNT;
    } while (next_protocol != s_active_protocol && !protocol_is_supported(next_protocol));

    next_protocol = normalize_protocol(next_protocol);
    if (next_protocol == s_active_protocol)
    {
        return false;
    }

    s_active_protocol = next_protocol;
    s_connection_state = BEBOPCORE_CONN_SEARCHING;
    app_state_set_protocol(s_active_protocol);
    storage_save_active_protocol(s_active_protocol);
    ESP_LOGI(TAG, "Protocol switched to %s", protocol_manager_get_active_name());
    return true;
}

void protocol_manager_enter_pairing(void)
{
    s_connection_state = BEBOPCORE_CONN_PAIRING;
    app_state_request_pairing();
    ESP_LOGI(TAG, "Pairing mode requested for protocol %s", protocol_manager_get_active_name());
}

void protocol_manager_reset_pairing(void)
{
    s_pairing_reset_counter++;
    storage_save_pairing_reset_counter(s_pairing_reset_counter);
    app_state_request_pairing_reset();
    s_connection_state = BEBOPCORE_CONN_SEARCHING;
    ESP_LOGW(TAG, "Pairing data reset requested for protocol %s", protocol_manager_get_active_name());
}

bebopCORE_protocol_t protocol_manager_get_active(void)
{
    return s_active_protocol;
}

bebopCORE_connection_state_t protocol_manager_get_connection_state(void)
{
    return s_connection_state;
}

const char *protocol_manager_get_active_name(void)
{
    return s_protocol_names[s_active_protocol];
}

bool protocol_manager_is_pairing_requested(void)
{
    return s_connection_state == BEBOPCORE_CONN_PAIRING;
}

void protocol_manager_set_connected(bool connected)
{
    s_connection_state = connected ? BEBOPCORE_CONN_CONNECTED : BEBOPCORE_CONN_SEARCHING;
    app_state_set_connected(connected);
}

void protocol_manager_process_report(const bebopCORE_report_t *report, bool shift_active)
{
    if (shift_active && s_connection_state == BEBOPCORE_CONN_PAIRING)
    {
        s_connection_state = BEBOPCORE_CONN_SEARCHING;
    }

    if (report_has_activity(report) && s_connection_state == BEBOPCORE_CONN_IDLE)
    {
        s_connection_state = BEBOPCORE_CONN_SEARCHING;
    }
}
