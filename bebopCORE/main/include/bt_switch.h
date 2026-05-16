#ifndef BEBOPCORE_BT_SWITCH_H
#define BEBOPCORE_BT_SWITCH_H

/*
 * bt_switch.h — Nintendo Switch Pro Controller emulation over Classic BT HID.
 *
 * Uses ESP-IDF's built-in esp_bt + esp_hidd (Classic BT, Bluedroid stack).
 * The Switch console acts as the HID Host; the ESP32 acts as HID Device.
 *
 * Report format mirrors the full input report 0x30 used by the Pro Controller:
 *   byte  0: report id (0x30)
 *   byte  1: timer counter
 *   byte  2: connection info + battery level nibble
 *   byte  3: right buttons
 *   byte  4: shared buttons (Plus/Minus/Home/Capture/stick clicks)
 *   byte  5: left buttons (D-Pad + L/ZL)
 *   bytes 6..11: left+right analogue sticks (3 bytes each, 12-bit values)
 *   bytes 12..47: vibration & IMU (zeroed in this implementation)
 */

#include <stdbool.h>
#include <stdint.h>

#include "bebopCORE_types.h"

/* Callback type: called from BT task with connection state changes */
typedef void (*bt_switch_connected_cb_t)(bool connected);

/* Initialise Classic BT and register HID device with Switch Pro Controller
 * profile.  Must be called before bt_switch_start().  Pass NULL to suppress
 * connection-change callbacks. */
void bt_switch_init(bt_switch_connected_cb_t on_connected);

/* Begin advertising / page-scanning so the Switch can find the controller.
 * Call after bt_switch_init(). */
void bt_switch_start(void);

/* Stop advertising and disconnect gracefully. */
void bt_switch_stop(void);

/* Returns true when a host is actively connected. */
bool bt_switch_is_connected(void);

/* Erase stored pairing bond.  Must be called while disconnected. */
void bt_switch_clear_bond(void);

/* Send a full input report (Report ID 0x30).
 * Should be called at ~8 ms intervals while connected. */
void bt_switch_send_report(const bebopCORE_report_t *report);

#endif /* BEBOPCORE_BT_SWITCH_H */
