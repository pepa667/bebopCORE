#ifndef BEBOPCORE_POWER_LED_H
#define BEBOPCORE_POWER_LED_H

#include <stdbool.h>
#include <stdint.h>

void power_led_init(void);
void power_led_set_battery(uint8_t battery_percent);
void power_led_set_charging(bool charging);
void power_led_tick(void);

#endif
