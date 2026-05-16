#ifndef BEBOPCORE_APP_STATE_H
#define BEBOPCORE_APP_STATE_H

#include <stdbool.h>
#include <stdint.h>

#include "bebopCORE_types.h"

typedef struct {
    uint32_t magic;
    uint32_t version;
    bebopCORE_protocol_t active_protocol;
    uint8_t battery_percent;
    bool charging;
    bool shift_active;
    bool protocol_change_requested;
    bool pairing_requested;
    bool pairing_reset_requested;
    bool recovery_requested;
    bool connected;
    uint8_t reserved[8];
} bebopCORE_runtime_state_t;

void app_state_init(void);
void app_state_set_protocol(bebopCORE_protocol_t protocol);
bebopCORE_protocol_t app_state_get_protocol(void);
void app_state_set_battery(uint8_t battery_percent, bool charging);
uint8_t app_state_get_battery_percent(void);
bool app_state_is_charging(void);
void app_state_set_shift_active(bool shift_active);
bool app_state_is_shift_active(void);
void app_state_request_protocol_change(void);
bool app_state_take_protocol_change_request(void);
void app_state_request_pairing(void);
bool app_state_take_pairing_request(void);
void app_state_request_pairing_reset(void);
bool app_state_take_pairing_reset_request(void);
void app_state_request_recovery(void);
bool app_state_take_recovery_request(void);
void app_state_set_connected(bool connected);
bool app_state_is_connected(void);

#endif
