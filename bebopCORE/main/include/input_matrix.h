#ifndef BEBOPCORE_INPUT_MATRIX_H
#define BEBOPCORE_INPUT_MATRIX_H

#include "bebopCORE_types.h"

void input_matrix_init(bool active_low);
void input_matrix_read(bebopCORE_input_state_t *state);

#endif
