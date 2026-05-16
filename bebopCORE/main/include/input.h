#ifndef BEBOPCORE_INPUT_H
#define BEBOPCORE_INPUT_H

#include <stdbool.h>
#include "bebopCORE_types.h"

typedef enum {
    BEBOPCORE_INPUT_BACKEND_GPIO = 0,
    BEBOPCORE_INPUT_BACKEND_MATRIX
} bebopCORE_input_backend_t;

typedef struct {
    bebopCORE_input_backend_t backend;
    bool active_low;
} bebopCORE_input_config_t;

void input_init(const bebopCORE_input_config_t *config);
void input_update(void);
const bebopCORE_input_state_t *input_get_state(void);
bool input_was_pressed(bebopCORE_button_id_t button);
bool input_is_pressed(bebopCORE_button_id_t button);
bool input_is_held(bebopCORE_button_id_t button, uint32_t hold_ms);
uint32_t input_get_hold_ms(bebopCORE_button_id_t button);
void input_build_report(bebopCORE_report_t *report, bebopCORE_protocol_t protocol, bool shift_active);

#endif
