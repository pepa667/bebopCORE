#ifndef BEBOPCORE_BATTERY_H
#define BEBOPCORE_BATTERY_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool enabled;
    uint8_t adc_gpio;
    float divider_ratio;
    float offset_volts;
    int charging_gpio;   /* GPIO for charger CHRG/STAT pin (active low). -1 = disabled */
} bebopCORE_battery_config_t;

void battery_init(const bebopCORE_battery_config_t *config);
void battery_update(void);
uint8_t battery_get_percent(void);
bool battery_is_charging(void);
bool battery_is_critical(void);

#endif
