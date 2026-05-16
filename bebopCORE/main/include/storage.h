#ifndef BEBOPCORE_STORAGE_H
#define BEBOPCORE_STORAGE_H

#include <stdbool.h>

#include "bebopCORE_types.h"

void storage_init(void);
void storage_save_active_protocol(bebopCORE_protocol_t protocol);
bebopCORE_protocol_t storage_load_active_protocol(void);
void storage_save_pairing_reset_counter(uint32_t value);
uint32_t storage_load_pairing_reset_counter(void);
bool storage_is_ready(void);

#endif
