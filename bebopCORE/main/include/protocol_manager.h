#ifndef BEBOPCORE_PROTOCOL_MANAGER_H
#define BEBOPCORE_PROTOCOL_MANAGER_H

#include <stdbool.h>
#include "bebopCORE_types.h"

void protocol_manager_init(void);
void protocol_manager_next(void);
void protocol_manager_enter_pairing(void);
void protocol_manager_reset_pairing(void);
bebopCORE_protocol_t protocol_manager_get_active(void);
bebopCORE_connection_state_t protocol_manager_get_connection_state(void);
const char *protocol_manager_get_active_name(void);
bool protocol_manager_is_pairing_requested(void);
void protocol_manager_set_connected(bool connected);
void protocol_manager_process_report(const bebopCORE_report_t *report, bool shift_active);

#endif
