/*
 * bt_switch.c — Nintendo Switch Pro Controller BT HID Device backend.
 *
 * Classic Bluetooth HID device via ESP-IDF Bluedroid stack (IDF 5.5).
 * Uses esp_bt_hid_device_* APIs from esp_hidd_api.h (part of 'bt' component).
 *
 * Lifecycle:
 *  bt_switch_init()  → registers callback → esp_bt_hid_device_init()
 *    → INIT_EVENT    → esp_bt_hid_device_register_app()
 *    → REGISTER_APP_EVENT → make discoverable
 *  OPEN_EVENT        → mark connected, call user callback
 *  bt_switch_send_report() → esp_bt_hid_device_send_report() at ~8 ms
 *  CLOSE_EVENT       → mark disconnected, resume advertising
 */

#include "bt_switch.h"

#include <stdlib.h>
#include <string.h>

#include "esp_bt.h"
#include "esp_bt_defs.h"
#include "esp_bt_device.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_hidd_api.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

static const char *TAG = "bebopCORE_bt_switch";

/* ---------- HID descriptor ------------------------------------------------ */
static const uint8_t s_procon_hid_desc[] = {
    0x05,
    0x01, /* Usage Page (Generic Desktop Ctrls)   */
    0x09,
    0x05, /* Usage (Game Pad)                     */
    0xA1,
    0x01, /* Collection (Application)             */
    0x06,
    0x01,
    0xFF, /*   Usage Page (Vendor 0xFF01)         */
    /* Report 0x21 */
    0x85,
    0x21,
    0x09,
    0x21,
    0x75,
    0x08,
    0x95,
    0x30,
    0x81,
    0x02,
    /* Report 0x30 – full input */
    0x85,
    0x30,
    0x09,
    0x30,
    0x75,
    0x08,
    0x95,
    0x30,
    0x81,
    0x02,
    /* Reports 0x31-0x33 */
    0x85,
    0x31,
    0x09,
    0x31,
    0x75,
    0x08,
    0x96,
    0x69,
    0x01,
    0x81,
    0x02,
    0x85,
    0x32,
    0x09,
    0x32,
    0x75,
    0x08,
    0x96,
    0x69,
    0x01,
    0x81,
    0x02,
    0x85,
    0x33,
    0x09,
    0x33,
    0x75,
    0x08,
    0x96,
    0x69,
    0x01,
    0x81,
    0x02,
    /* Report 0x3F – simple button + hat + 4-axis */
    0x85,
    0x3F,
    0x05,
    0x09,
    0x19,
    0x01,
    0x29,
    0x10,
    0x15,
    0x00,
    0x25,
    0x01,
    0x75,
    0x01,
    0x95,
    0x10,
    0x81,
    0x02,
    0x05,
    0x01,
    0x09,
    0x39,
    0x15,
    0x00,
    0x25,
    0x07,
    0x75,
    0x04,
    0x95,
    0x01,
    0x81,
    0x42,
    0x05,
    0x09,
    0x75,
    0x04,
    0x95,
    0x01,
    0x81,
    0x01,
    0x05,
    0x01,
    0x09,
    0x30,
    0x09,
    0x31,
    0x09,
    0x33,
    0x09,
    0x34,
    0x16,
    0x00,
    0x00,
    0x27,
    0xFF,
    0xFF,
    0x00,
    0x00,
    0x75,
    0x10,
    0x95,
    0x04,
    0x81,
    0x02,
    0x06,
    0x01,
    0xFF,
    /* Output reports */
    0x85,
    0x01,
    0x09,
    0x01,
    0x75,
    0x08,
    0x95,
    0x30,
    0x91,
    0x02,
    0x85,
    0x10,
    0x09,
    0x10,
    0x75,
    0x08,
    0x95,
    0x09,
    0x91,
    0x02,
    0x85,
    0x11,
    0x09,
    0x11,
    0x75,
    0x08,
    0x95,
    0x30,
    0x91,
    0x02,
    0x85,
    0x12,
    0x09,
    0x12,
    0x75,
    0x08,
    0x95,
    0x30,
    0x91,
    0x02,
    0xC0,
};

/* ---------- HID app / QoS params ----------------------------------------- */
static esp_hidd_app_param_t s_app_param = {
    .name = "Pro Controller",
    .description = "Nintendo Switch Pro Controller",
    .provider = "Nintendo",
    .subclass = 0x08,
    .desc_list = (uint8_t *)s_procon_hid_desc,
    .desc_list_len = sizeof(s_procon_hid_desc),
};

static esp_hidd_qos_param_t s_qos_in = {
    .service_type = 0,
    .token_rate = 0,
    .token_bucket_size = 0,
    .peak_bandwidth = 0,
    .access_latency = 0xFFFFFFFF,
    .delay_variation = 0xFFFFFFFF,
};
static esp_hidd_qos_param_t s_qos_out = {
    .service_type = 0,
    .token_rate = 0,
    .token_bucket_size = 0,
    .peak_bandwidth = 0,
    .access_latency = 0xFFFFFFFF,
    .delay_variation = 0xFFFFFFFF,
};

/* ---------- report packing (Report ID 0x30, 48-byte payload) -------------- */
#define RPT_PAYLOAD 48

static inline uint16_t axis_to_12bit(int16_t v)
{
    int32_t s = (int32_t)v + 32768;
    return (uint16_t)((s * 4095) / 65535);
}

static void pack_full_report(const bebopCORE_report_t *rpt, uint8_t buf[RPT_PAYLOAD],
                             uint8_t timer, uint8_t bat_nibble)
{
    memset(buf, 0, RPT_PAYLOAD);
    buf[0] = timer;
    buf[1] = (uint8_t)(((bat_nibble & 0x0F) << 4) | 0x01);

    /* Byte 2: right buttons */
    buf[2] = (uint8_t)((rpt->button_y ? 0x01 : 0) |
                       (rpt->button_x ? 0x02 : 0) |
                       (rpt->button_b ? 0x04 : 0) |
                       (rpt->button_a ? 0x08 : 0) |
                       (rpt->trigger_r ? 0x40 : 0) |
                       (rpt->trigger_zr ? 0x80 : 0));

    /* Byte 3: shared (Plus=start, Minus=select, Home, Capture, sticks) */
    buf[3] = (uint8_t)((rpt->button_select ? 0x01 : 0) |
                       (rpt->button_start ? 0x02 : 0) |
                       (rpt->button_stick_right ? 0x04 : 0) |
                       (rpt->button_stick_left ? 0x08 : 0) |
                       (rpt->button_home ? 0x10 : 0) |
                       (rpt->button_capture ? 0x20 : 0));

    /* Byte 4: left buttons */
    buf[4] = (uint8_t)((rpt->dpad_down ? 0x01 : 0) |
                       (rpt->dpad_up ? 0x02 : 0) |
                       (rpt->dpad_right ? 0x04 : 0) |
                       (rpt->dpad_left ? 0x08 : 0) |
                       (rpt->trigger_l ? 0x40 : 0) |
                       (rpt->trigger_zl ? 0x80 : 0));

    /* Left stick: bytes 5..7 */
    uint16_t lx = axis_to_12bit(rpt->lx);
    uint16_t ly = axis_to_12bit(rpt->ly);
    buf[5] = (uint8_t)(lx & 0xFF);
    buf[6] = (uint8_t)(((lx >> 8) & 0x0F) | ((ly & 0x0F) << 4));
    buf[7] = (uint8_t)(ly >> 4);

    /* Right stick: bytes 8..10 */
    uint16_t rx = axis_to_12bit(rpt->rx);
    uint16_t ry = axis_to_12bit(rpt->ry);
    buf[8] = (uint8_t)(rx & 0xFF);
    buf[9] = (uint8_t)(((rx >> 8) & 0x0F) | ((ry & 0x0F) << 4));
    buf[10] = (uint8_t)(ry >> 4);
    /* bytes 11..47: vibration ack + IMU (zeroed) */
}

/* ---------- state --------------------------------------------------------- */
static bt_switch_connected_cb_t s_connected_cb = NULL;
static volatile bool s_connected = false;
static uint8_t s_timer = 0;
static uint8_t s_last_rpt_buf[RPT_PAYLOAD];

/* ---------- HIDD callback ------------------------------------------------- */
static void bt_switch_hidd_cb(esp_hidd_cb_event_t event, esp_hidd_cb_param_t *param)
{
    switch (event)
    {
    case ESP_HIDD_INIT_EVT:
        if (param->init.status == ESP_HIDD_SUCCESS)
        {
            ESP_LOGI(TAG, "HID init ok — registering app");
            vTaskDelay(pdMS_TO_TICKS(2000));
            esp_bt_hid_device_register_app(&s_app_param, &s_qos_in, &s_qos_out);
        }
        else
        {
            ESP_LOGE(TAG, "HID init failed: %d", (int)param->init.status);
        }
        break;

    case ESP_HIDD_REGISTER_APP_EVT:
        if (param->register_app.status == ESP_HIDD_SUCCESS)
        {
            ESP_LOGI(TAG, "HID app registered — advertising");
            esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
            if (param->register_app.in_use)
            {
                ESP_LOGI(TAG, "Virtual cable: reconnecting to last host");
                esp_bt_hid_device_connect(param->register_app.bd_addr);
            }
        }
        else
        {
            ESP_LOGE(TAG, "HID register_app failed: %d", (int)param->register_app.status);
        }
        break;

    case ESP_HIDD_OPEN_EVT:
        if (param->open.status == ESP_HIDD_SUCCESS)
        {
            if (param->open.conn_status == ESP_HIDD_CONN_STATE_CONNECTING)
            {
                ESP_LOGI(TAG, "HID connecting...");
            }
            else if (param->open.conn_status == ESP_HIDD_CONN_STATE_CONNECTED)
            {
                ESP_LOGI(TAG, "HID connected");
                s_connected = true;
                esp_bt_gap_set_scan_mode(ESP_BT_NON_CONNECTABLE, ESP_BT_NON_DISCOVERABLE);
                if (s_connected_cb)
                    s_connected_cb(true);
            }
        }
        else
        {
            ESP_LOGW(TAG, "HID open error: %d", (int)param->open.status);
        }
        break;

    case ESP_HIDD_CLOSE_EVT:
        if (param->close.conn_status == ESP_HIDD_CONN_STATE_DISCONNECTING)
        {
            ESP_LOGI(TAG, "HID disconnecting...");
        }
        else if (param->close.conn_status == ESP_HIDD_CONN_STATE_DISCONNECTED)
        {
            ESP_LOGI(TAG, "HID disconnected");
            s_connected = false;
            if (s_connected_cb)
                s_connected_cb(false);
            esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
        }
        break;

    case ESP_HIDD_GET_REPORT_EVT:
        /* Respond with last known input report */
        esp_bt_hid_device_send_report(ESP_HIDD_REPORT_TYPE_INTRDATA,
                                      param->get_report.report_id,
                                      RPT_PAYLOAD, s_last_rpt_buf);
        break;

    case ESP_HIDD_SET_REPORT_EVT:
        /* Output report from host (e.g. rumble, LED commands) */
        ESP_LOGD(TAG, "SET_REPORT id=0x%02x len=%d",
                 param->set_report.report_id, param->set_report.len);
        break;

    case ESP_HIDD_INTR_DATA_EVT:
        /* Data on interrupt channel (host output reports, e.g. rumble) */
        ESP_LOGD(TAG, "INTR_DATA id=0x%02x len=%d",
                 param->intr_data.report_id, param->intr_data.len);
        break;

    case ESP_HIDD_SET_PROTOCOL_EVT:
        ESP_LOGI(TAG, "SET_PROTOCOL mode=%d", (int)param->set_protocol.protocol_mode);
        break;

    case ESP_HIDD_DEINIT_EVT:
        ESP_LOGI(TAG, "HID deinit ok");
        break;

    case ESP_HIDD_SEND_REPORT_EVT:
    case ESP_HIDD_REPORT_ERR_EVT:
        break;

    default:
        break;
    }
}

/* ---------- GAP callback -------------------------------------------------- */
static void bt_switch_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    switch (event)
    {
    case ESP_BT_GAP_AUTH_CMPL_EVT:
        if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS)
        {
            ESP_LOGI(TAG, "Auth OK");
        }
        else
        {
            ESP_LOGW(TAG, "Auth failed: %d", param->auth_cmpl.stat);
        }
        break;
    case ESP_BT_GAP_PIN_REQ_EVT:
    {
        esp_bt_pin_code_t pin = {0, 0, 0, 0};
        esp_bt_gap_pin_reply(param->pin_req.bda, true, 4, pin);
        break;
    }
    default:
        break;
    }
}

/* ---------- public API ---------------------------------------------------- */
void bt_switch_init(bt_switch_connected_cb_t on_connected)
{
    s_connected_cb = on_connected;

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    bt_cfg.mode = ESP_BT_MODE_CLASSIC_BT;
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT));
    esp_bluedroid_config_t bluedroid_cfg = BT_BLUEDROID_INIT_CONFIG_DEFAULT();
    bluedroid_cfg.ssp_en = false;
    ESP_ERROR_CHECK(esp_bluedroid_init_with_cfg(&bluedroid_cfg));
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    esp_bt_gap_register_callback(bt_switch_gap_cb);
    esp_bt_gap_set_device_name("Pro Controller");

    /* COD = Peripheral | Gamepad (0x002508) */
    esp_bt_cod_t hid_cod;
    uint32_t cod_val = 0x002508;
    memcpy(&hid_cod, &cod_val, sizeof(uint32_t));
    esp_bt_gap_set_cod(hid_cod, ESP_BT_INIT_COD);

    /* Legacy variable-PIN pairing (no SSP) */
    esp_bt_pin_type_t pin_type = ESP_BT_PIN_TYPE_VARIABLE;
    esp_bt_pin_code_t pin_code;
    esp_bt_gap_set_pin(pin_type, 0, pin_code);

    ESP_ERROR_CHECK(esp_bt_hid_device_register_callback(bt_switch_hidd_cb));
    ESP_ERROR_CHECK(esp_bt_hid_device_init());
    /* App registration triggered inside INIT_EVENT → REGISTER_APP_EVENT */
    ESP_LOGI(TAG, "bt_switch_init done");
}

void bt_switch_start(void)
{
    /* Advertising is set inside REGISTER_APP_EVENT; this is a no-op placeholder */
    ESP_LOGI(TAG, "bt_switch_start (advertising set via REGISTER_APP_EVENT)");
}

void bt_switch_stop(void)
{
    esp_bt_gap_set_scan_mode(ESP_BT_NON_CONNECTABLE, ESP_BT_NON_DISCOVERABLE);
    esp_bt_hid_device_unregister_app();
    esp_bt_hid_device_deinit();
    esp_bluedroid_disable();
    esp_bluedroid_deinit();
    esp_bt_controller_disable();
    esp_bt_controller_deinit();
    s_connected = false;
    ESP_LOGI(TAG, "bt_switch stopped");
}

bool bt_switch_is_connected(void)
{
    return s_connected;
}

void bt_switch_clear_bond(void)
{
    int count = esp_bt_gap_get_bond_device_num();
    if (count <= 0)
        return;
    esp_bd_addr_t *list = malloc((size_t)count * sizeof(esp_bd_addr_t));
    if (!list)
        return;
    esp_bt_gap_get_bond_device_list(&count, list);
    for (int i = 0; i < count; i++)
    {
        esp_bt_gap_remove_bond_device(list[i]);
    }
    free(list);
    ESP_LOGI(TAG, "Cleared %d BT bond(s)", count);
}

void bt_switch_send_report(const bebopCORE_report_t *report)
{
    if (!s_connected)
        return;
    uint8_t buf[RPT_PAYLOAD];
    pack_full_report(report, buf, s_timer++, 0x8 /* battery placeholder */);
    memcpy(s_last_rpt_buf, buf, RPT_PAYLOAD);
    esp_bt_hid_device_send_report(ESP_HIDD_REPORT_TYPE_INTRDATA,
                                  0x30, RPT_PAYLOAD, buf);
}
