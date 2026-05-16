#include <string.h>

#include "app_state.h"

#define BEBOPCORE_STATE_MAGIC 0x42434F52u
#define BEBOPCORE_STATE_VERSION 1u

static bebopCORE_runtime_state_t s_state;

void app_state_init(void)
{
    memset(&s_state, 0, sizeof(s_state));
    s_state.magic = BEBOPCORE_STATE_MAGIC;
    s_state.version = BEBOPCORE_STATE_VERSION;
    s_state.active_protocol = BEBOPCORE_PROTOCOL_SWITCH;
    s_state.battery_percent = 100;
}

void app_state_set_protocol(bebopCORE_protocol_t protocol)
{
    s_state.active_protocol = protocol;
}

bebopCORE_protocol_t app_state_get_protocol(void)
{
    return s_state.active_protocol;
}

void app_state_set_battery(uint8_t battery_percent, bool charging)
{
    s_state.battery_percent = battery_percent;
    s_state.charging = charging;
}

uint8_t app_state_get_battery_percent(void)
{
    return s_state.battery_percent;
}

bool app_state_is_charging(void)
{
    return s_state.charging;
}

void app_state_set_shift_active(bool shift_active)
{
    s_state.shift_active = shift_active;
}

bool app_state_is_shift_active(void)
{
    return s_state.shift_active;
}

void app_state_request_protocol_change(void)
{
    s_state.protocol_change_requested = true;
}

bool app_state_take_protocol_change_request(void)
{
    bool value = s_state.protocol_change_requested;
    s_state.protocol_change_requested = false;
    return value;
}

void app_state_request_pairing(void)
{
    s_state.pairing_requested = true;
}

bool app_state_take_pairing_request(void)
{
    bool value = s_state.pairing_requested;
    s_state.pairing_requested = false;
    return value;
}

void app_state_request_pairing_reset(void)
{
    s_state.pairing_reset_requested = true;
}

bool app_state_take_pairing_reset_request(void)
{
    bool value = s_state.pairing_reset_requested;
    s_state.pairing_reset_requested = false;
    return value;
}

void app_state_request_recovery(void)
{
    s_state.recovery_requested = true;
}

bool app_state_take_recovery_request(void)
{
    bool value = s_state.recovery_requested;
    s_state.recovery_requested = false;
    return value;
}

void app_state_set_connected(bool connected)
{
    s_state.connected = connected;
}

bool app_state_is_connected(void)
{
    return s_state.connected;
}
