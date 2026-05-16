#ifndef BEBOPCORE_STATUS_LEDS_H
#define BEBOPCORE_STATUS_LEDS_H

#include <stdint.h>
#include "bebopCORE_types.h"

void status_leds_init(void);
void status_leds_set_protocol(bebopCORE_protocol_t protocol);
void status_leds_set_connection(bebopCORE_connection_state_t state);
void status_leds_set_battery(uint8_t battery_percent);
void status_leds_set_shift(bool enabled);
void status_leds_set_protocol_change_pulse(void);
void status_leds_set_pairing_pulse(void);
void status_leds_tick(void);

#endif
