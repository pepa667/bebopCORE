#ifndef BEBOPCORE_INPUT_GPIO_H
#define BEBOPCORE_INPUT_GPIO_H

#include "bebopCORE_types.h"

void input_gpio_init(bool active_low);
void input_gpio_read(bebopCORE_input_state_t *state);

#endif
