#ifndef BEBOPCORE_BT_MANAGER_H
#define BEBOPCORE_BT_MANAGER_H

/*
 * bt_manager.h — Protocol-aware BT dispatcher.
 *
 * Selects the appropriate BT backend (Switch / XInput / DInput / Generic)
 * based on the active protocol, starts it, handles report forwarding,
 * and calls back into protocol_manager with connection state changes.
 *
 * Only one backend is active at a time.  Switching protocol requires calling
 * bt_manager_switch_protocol() which tears down the current backend and
 * brings up the new one.
 */

#include <stdbool.h>
#include "bebopCORE_types.h"

/* Initialise and start the backend for the given protocol.
 * Must be called once after storage_init() and app_state_init(). */
void bt_manager_init(bebopCORE_protocol_t protocol);

/* Tear down current backend and start the new one.
 * Should be called whenever protocol_manager_next() is used. */
void bt_manager_switch_protocol(bebopCORE_protocol_t new_protocol);

/* Returns true when the active backend reports a host is connected. */
bool bt_manager_is_connected(void);

/* Erase pairing/bond data for the currently active protocol. */
void bt_manager_clear_pairing(void);

/* Forward an input report to the active backend.
 * Call at the same ~10 ms tick as the rest of the main loop. */
void bt_manager_send_report(const bebopCORE_report_t *report);

#endif /* BEBOPCORE_BT_MANAGER_H */
