/*
 * bt_manager.c — Protocol-aware BT dispatcher.
 *
 * Currently wires the Switch backend fully.
 * XInput / DInput / Generic backends are stubs that will be filled in as
 * separate bt_xinput.c / bt_dinput.c / bt_ble_hid.c modules.
 */

#include "bt_manager.h"

#include "esp_log.h"
#include "protocol_manager.h"
#include "bt_switch.h"

static const char *TAG = "bebopCORE_bt_mgr";

static bebopCORE_protocol_t s_current_protocol = BEBOPCORE_PROTOCOL_COUNT;

/* ---------- common connected callback ------------------------------------- */

static void on_connected(bool connected)
{
    protocol_manager_set_connected(connected);
    ESP_LOGI(TAG, "BT %s", connected ? "connected" : "disconnected");
}

/* ---------- internal helpers ---------------------------------------------- */

static void start_backend(bebopCORE_protocol_t proto)
{
    switch (proto)
    {
    case BEBOPCORE_PROTOCOL_SWITCH:
        bt_switch_init(on_connected);
        bt_switch_start();
        break;

    case BEBOPCORE_PROTOCOL_XINPUT:
    case BEBOPCORE_PROTOCOL_DINPUT:
    case BEBOPCORE_PROTOCOL_GENERIC:
        /* TODO: implement bt_xinput / bt_dinput / bt_ble_hid */
        ESP_LOGW(TAG, "Protocol %d BT backend not yet implemented", (int)proto);
        break;

    default:
        break;
    }
    s_current_protocol = proto;
}

static void stop_backend(bebopCORE_protocol_t proto)
{
    switch (proto)
    {
    case BEBOPCORE_PROTOCOL_SWITCH:
        bt_switch_stop();
        break;
    default:
        break;
    }
}

/* ---------- public API ---------------------------------------------------- */

void bt_manager_init(bebopCORE_protocol_t protocol)
{
    ESP_LOGI(TAG, "bt_manager_init protocol=%d", (int)protocol);
    start_backend(protocol);
}

void bt_manager_switch_protocol(bebopCORE_protocol_t new_protocol)
{
    if (new_protocol == s_current_protocol)
    {
        return;
    }
    ESP_LOGI(TAG, "Switching BT backend %d → %d", (int)s_current_protocol, (int)new_protocol);
    if (s_current_protocol < BEBOPCORE_PROTOCOL_COUNT)
    {
        stop_backend(s_current_protocol);
    }
    start_backend(new_protocol);
}

bool bt_manager_is_connected(void)
{
    switch (s_current_protocol)
    {
    case BEBOPCORE_PROTOCOL_SWITCH:
        return bt_switch_is_connected();
    default:
        return false;
    }
}

void bt_manager_clear_pairing(void)
{
    switch (s_current_protocol)
    {
    case BEBOPCORE_PROTOCOL_SWITCH:
        bt_switch_clear_bond();
        break;
    default:
        break;
    }
}

void bt_manager_send_report(const bebopCORE_report_t *report)
{
    switch (s_current_protocol)
    {
    case BEBOPCORE_PROTOCOL_SWITCH:
        bt_switch_send_report(report);
        break;
    default:
        break;
    }
}
