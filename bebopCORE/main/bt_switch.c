/*
 * bt_switch.c — Nintendo Switch Pro Controller BT HID Device backend.
 *
 * Classic Bluetooth HID device via ESP-IDF Bluedroid stack (IDF 5.5).
 * Uses the higher-level esp_hid wrapper so the BT HID registration path
 * matches the working HOJA backend more closely.
 */

#include "bt_switch.h"

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "esp_bt.h"
#include "esp_bt_defs.h"
#include "esp_bt_device.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_hidd.h"
#include "esp_hidd_api.h"
#include "esp_idf_version.h"
#include "esp_log.h"
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 5, 0) && CONFIG_BT_CLASSIC_ENABLED && CONFIG_BT_HID_DEVICE_ENABLED
#include "esp_sdp_api.h"
#endif
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 5, 0) && CONFIG_BT_CLASSIC_ENABLED && CONFIG_BT_HID_DEVICE_ENABLED
#define BEBOPCORE_BT_SDP_ENABLED 1
#else
#define BEBOPCORE_BT_SDP_ENABLED 0
#endif

#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 5, 0)
#define BEBOPCORE_SDP_SUCCESS 0x0000
#define BEBOPCORE_SDP_MAX_ATTR_LEN 400
#define BEBOPCORE_DI_VENDOR_ID_SOURCE_BTSIG 0x0001
#define BEBOPCORE_BTM_PM_MD_ACTIVE 0x00
#define BEBOPCORE_BTM_PM_SET_ONLY_ID 0x80
#define BEBOPCORE_HCI_ENABLE_MASTER_SLAVE_SWITCH 0x0001

typedef struct
{
    uint16_t vendor;
    uint16_t vendor_id_source;
    uint16_t product;
    uint16_t version;
    uint8_t primary_record;
    char client_executable_url[BEBOPCORE_SDP_MAX_ATTR_LEN];
    char service_description[BEBOPCORE_SDP_MAX_ATTR_LEN];
    char documentation_url[BEBOPCORE_SDP_MAX_ATTR_LEN];
} bebopCORE_sdp_di_record_t;

typedef uint8_t bebopCORE_btm_status_t;

typedef struct
{
    uint16_t max;
    uint16_t min;
    uint16_t attempt;
    uint16_t timeout;
    uint8_t mode;
} bebopCORE_btm_pm_pwr_md_t;

uint16_t SDP_SetLocalDiRecord(bebopCORE_sdp_di_record_t *p_device_info, uint32_t *p_handle);
bebopCORE_btm_status_t BTM_SetLinkPolicy(esp_bd_addr_t remote_bda, uint16_t *settings);
bebopCORE_btm_status_t BTM_SetPowerMode(uint8_t pm_id, esp_bd_addr_t remote_bda,
                                        bebopCORE_btm_pm_pwr_md_t *p_mode);
#endif

static const char *TAG = "bebopCORE_bt_switch";

#define HID_VEND_NSPRO 0x057E
#define HID_PROD_NSPRO 0x2009
#define HID_VER_NSPRO 0x0100

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

/* ---------- HID device config -------------------------------------------- */
static esp_hid_raw_report_map_t s_report_maps[] = {
    {
        .data = s_procon_hid_desc,
        .len = sizeof(s_procon_hid_desc),
    },
};

static esp_hid_device_config_t s_hid_config = {
    .vendor_id = HID_VEND_NSPRO,
    .product_id = HID_PROD_NSPRO,
    .version = HID_VER_NSPRO,
    .device_name = "Pro Controller",
    .manufacturer_name = "Nintendo",
    .serial_number = "000000",
    .report_maps = s_report_maps,
    .report_maps_len = 1,
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
static volatile bool s_force_active_acl_pending = false;
static uint8_t s_timer = 0;
static uint8_t s_last_rpt_buf[RPT_PAYLOAD];
static esp_hidd_dev_t *s_hid_dev = NULL;

#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 5, 0)
typedef struct
{
    void *dev;
} bebopCORE_hidd_wrapper_dev_t;

typedef struct
{
    void *dev;
    esp_event_loop_handle_t event_loop_handle;
    esp_hid_device_config_t config;
    uint16_t appearance;
    bool registered;
    bool connected;
    esp_bd_addr_t remote_bda;
    uint8_t bat_level;
    uint8_t control;
    uint8_t protocol_mode;
} bebopCORE_bt_hidd_dev_t;

static uint8_t bt_switch_translate_protocol_mode(uint8_t raw_protocol_mode)
{
    switch (raw_protocol_mode)
    {
    case ESP_HIDD_REPORT_MODE:
        return ESP_HID_PROTOCOL_MODE_REPORT;
    case ESP_HIDD_BOOT_MODE:
        return ESP_HID_PROTOCOL_MODE_BOOT;
    default:
        return raw_protocol_mode;
    }
}

static const char *bt_switch_raw_protocol_mode_name(uint8_t protocol_mode)
{
    switch (protocol_mode)
    {
    case ESP_HIDD_REPORT_MODE:
        return "REPORT";
    case ESP_HIDD_BOOT_MODE:
        return "BOOT";
    default:
        return "UNKNOWN";
    }
}

static const char *bt_switch_internal_protocol_mode_name(uint8_t protocol_mode)
{
    switch (protocol_mode)
    {
    case ESP_HID_PROTOCOL_MODE_REPORT:
        return "REPORT";
    case ESP_HID_PROTOCOL_MODE_BOOT:
        return "BOOT";
    default:
        return "UNKNOWN";
    }
}

static void bt_switch_sync_protocol_mode(uint8_t raw_protocol_mode)
{
    bebopCORE_hidd_wrapper_dev_t *wrapper_dev;
    bebopCORE_bt_hidd_dev_t *bt_dev;

    if (!s_hid_dev)
    {
        return;
    }

    wrapper_dev = (bebopCORE_hidd_wrapper_dev_t *)s_hid_dev;
    bt_dev = (bebopCORE_bt_hidd_dev_t *)wrapper_dev->dev;
    if (!bt_dev)
    {
        return;
    }

    bt_dev->protocol_mode = bt_switch_translate_protocol_mode(raw_protocol_mode);
}

static bool bt_switch_get_remote_bda(esp_bd_addr_t remote_bda)
{
    bebopCORE_hidd_wrapper_dev_t *wrapper_dev;
    bebopCORE_bt_hidd_dev_t *bt_dev;

    if (!s_hid_dev)
    {
        return false;
    }

    wrapper_dev = (bebopCORE_hidd_wrapper_dev_t *)s_hid_dev;
    if (!wrapper_dev || !wrapper_dev->dev)
    {
        return false;
    }

    bt_dev = (bebopCORE_bt_hidd_dev_t *)wrapper_dev->dev;
    memcpy(remote_bda, bt_dev->remote_bda, sizeof(esp_bd_addr_t));
    return true;
}

static void bt_switch_force_active_acl(const char *reason)
{
    esp_bd_addr_t remote_bda;
    uint16_t link_policy = BEBOPCORE_HCI_ENABLE_MASTER_SLAVE_SWITCH;
    bebopCORE_btm_pm_pwr_md_t pwr_md = {0};
    bebopCORE_btm_status_t link_status;
    bebopCORE_btm_status_t power_status;

    if (!bt_switch_get_remote_bda(remote_bda))
    {
        ESP_LOGW(TAG, "force active ACL skipped (%s): no remote BDA", reason);
        return;
    }

    link_status = BTM_SetLinkPolicy(remote_bda, &link_policy);

    pwr_md.mode = BEBOPCORE_BTM_PM_MD_ACTIVE;
    power_status = BTM_SetPowerMode(BEBOPCORE_BTM_PM_SET_ONLY_ID, remote_bda, &pwr_md);

    ESP_LOGI(TAG,
             "Force active ACL (%s): %02x:%02x:%02x:%02x:%02x:%02x policy=0x%04x link_status=%u power_status=%u",
             reason,
             remote_bda[0], remote_bda[1], remote_bda[2],
             remote_bda[3], remote_bda[4], remote_bda[5],
             (unsigned int)link_policy,
             (unsigned int)link_status,
             (unsigned int)power_status);
}
#endif

#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 5, 0)
static uint32_t s_di_record_handle = 0;

static void bt_switch_register_device_id_record(void)
{
    bebopCORE_sdp_di_record_t di_record = {
        .vendor = HID_VEND_NSPRO,
        .vendor_id_source = BEBOPCORE_DI_VENDOR_ID_SOURCE_BTSIG,
        .product = HID_PROD_NSPRO,
        .version = HID_VER_NSPRO,
        .primary_record = true,
    };
    uint32_t handle = 0;
    uint16_t status = SDP_SetLocalDiRecord(&di_record, &handle);

    if (status != BEBOPCORE_SDP_SUCCESS)
    {
        ESP_LOGW(TAG, "Device ID SDP record failed: 0x%04x", status);
        return;
    }

    s_di_record_handle = handle;
    ESP_LOGI(TAG,
             "Device ID SDP record ok: handle=0x%08" PRIx32 " vid=0x%04x pid=0x%04x ver=0x%04x",
             s_di_record_handle,
             HID_VEND_NSPRO,
             HID_PROD_NSPRO,
             HID_VER_NSPRO);
}
#endif

#if !CONFIG_BT_SSP_ENABLED
static void bt_switch_fill_pin_code(esp_bt_pin_code_t pin)
{
    memset(pin, 0, ESP_BT_PIN_CODE_LEN);

#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 1, 0)
    pin[0] = '1';
    pin[1] = '2';
    pin[2] = '3';
    pin[3] = '4';
#endif
}

static uint8_t bt_switch_pin_length(void)
{
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 1, 0)
    return 4;
#else
    return 0;
#endif
}
#endif

static void bt_switch_set_scan_mode(esp_bt_connection_mode_t c_mode,
                                    esp_bt_discovery_mode_t d_mode,
                                    const char *reason)
{
    esp_err_t ret = esp_bt_gap_set_scan_mode(c_mode, d_mode);
    if (ret != ESP_OK)
    {
        ESP_LOGW(TAG, "set_scan_mode(%s) failed: %s", reason, esp_err_to_name(ret));
    }
}

static bool bt_switch_try_reconnect_bonded_host(void)
{
    int count = esp_bt_gap_get_bond_device_num();
    if (count <= 0)
    {
        return false;
    }

    esp_bd_addr_t *list = calloc((size_t)count, sizeof(esp_bd_addr_t));
    if (!list)
    {
        ESP_LOGW(TAG, "Failed to allocate bonded-device list");
        return false;
    }

    int entries = count;
    esp_err_t ret = esp_bt_gap_get_bond_device_list(&entries, list);
    if (ret != ESP_OK || entries <= 0)
    {
        ESP_LOGW(TAG, "Failed to read bonded-device list: %s", esp_err_to_name(ret));
        free(list);
        return false;
    }

    ESP_LOGI(TAG, "Reconnecting bonded host %02x:%02x:%02x:%02x:%02x:%02x",
             list[0][0], list[0][1], list[0][2], list[0][3], list[0][4], list[0][5]);
    ret = esp_bt_hid_device_connect(list[0]);
    free(list);

    if (ret != ESP_OK)
    {
        ESP_LOGW(TAG, "Bonded-host reconnect failed: %s", esp_err_to_name(ret));
        return false;
    }

    return true;
}

#if BEBOPCORE_BT_SDP_ENABLED
static void bt_switch_sdp_cb(esp_sdp_cb_event_t event, esp_sdp_cb_param_t *param)
{
    switch (event)
    {
    case ESP_SDP_INIT_EVT:
        if (param->init.status == ESP_SDP_SUCCESS)
        {
            esp_bluetooth_sdp_dip_record_t dip_record = {
                .hdr = {
                    .type = ESP_SDP_TYPE_DIP_SERVER,
                },
                .vendor = HID_VEND_NSPRO,
                .vendor_id_source = ESP_SDP_VENDOR_ID_SRC_BT,
                .product = HID_PROD_NSPRO,
                .version = HID_VER_NSPRO,
                .primary_record = true,
            };
            esp_err_t ret = esp_sdp_create_record((esp_bluetooth_sdp_record_t *)&dip_record);
            if (ret != ESP_OK)
            {
                ESP_LOGE(TAG, "SDP create DIP record failed: %s", esp_err_to_name(ret));
            }
            else
            {
                ESP_LOGI(TAG, "SDP initialized, creating DIP record");
            }
        }
        else
        {
            ESP_LOGE(TAG, "SDP init failed: %d", (int)param->init.status);
        }
        break;

    case ESP_SDP_CREATE_RECORD_COMP_EVT:
        ESP_LOGI(TAG, "SDP record created: status=%d handle=0x%x",
                 (int)param->create_record.status,
                 param->create_record.record_handle);
        break;

    case ESP_SDP_REMOVE_RECORD_COMP_EVT:
        ESP_LOGI(TAG, "SDP record removed: status=%d", (int)param->remove_record.status);
        break;

    case ESP_SDP_DEINIT_EVT:
        ESP_LOGI(TAG, "SDP deinit: status=%d", (int)param->deinit.status);
        break;

    default:
        break;
    }
}
#endif

/* ---------- HIDD callback ------------------------------------------------- */
static void bt_switch_hidd_cb(void *handler_args, esp_event_base_t base, int32_t id, void *event_data)
{
    (void)handler_args;
    (void)base;

    esp_hidd_event_t event = (esp_hidd_event_t)id;
    esp_hidd_event_data_t *param = (esp_hidd_event_data_t *)event_data;

    if (param == NULL)
    {
        return;
    }

    switch (event)
    {
    case ESP_HIDD_START_EVENT:
        if (param->start.status == ESP_OK)
        {
            ESP_LOGI(TAG, "HID start ok");
            if (!bt_switch_try_reconnect_bonded_host())
            {
                ESP_LOGI(TAG, "HID ready — advertising");
                bt_switch_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE,
                                        "start discoverable");
            }
        }
        else
        {
            ESP_LOGE(TAG, "HID start failed: %s", esp_err_to_name(param->start.status));
        }
        break;

    case ESP_HIDD_CONNECT_EVENT:
        if (param->connect.status == ESP_OK)
        {
            ESP_LOGI(TAG, "HID connected");
            s_connected = true;
            s_force_active_acl_pending = true;
            bt_switch_set_scan_mode(ESP_BT_NON_CONNECTABLE, ESP_BT_NON_DISCOVERABLE,
                                    "connected hidden");
            if (s_connected_cb)
                s_connected_cb(true);
        }
        else
        {
            ESP_LOGW(TAG, "HID connect error: %s", esp_err_to_name(param->connect.status));
        }
        break;

    case ESP_HIDD_PROTOCOL_MODE_EVENT:
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 5, 0)
    {
        uint8_t raw_protocol_mode = param->protocol_mode.protocol_mode;
        uint8_t internal_protocol_mode = bt_switch_translate_protocol_mode(raw_protocol_mode);

        bt_switch_sync_protocol_mode(raw_protocol_mode);
        ESP_LOGI(TAG,
                 "SET_PROTOCOL raw=%d (%s) internal=%d (%s)",
                 (int)raw_protocol_mode,
                 bt_switch_raw_protocol_mode_name(raw_protocol_mode),
                 (int)internal_protocol_mode,
                 bt_switch_internal_protocol_mode_name(internal_protocol_mode));
        break;
    }
#else
        ESP_LOGI(TAG, "SET_PROTOCOL mode=%d", (int)param->protocol_mode.protocol_mode);
        break;
#endif

    case ESP_HIDD_OUTPUT_EVENT:
        ESP_LOGI(TAG, "OUTPUT id=0x%02x len=%u map=%u",
                 (unsigned int)param->output.report_id,
                 (unsigned int)param->output.length,
                 (unsigned int)param->output.map_index);
        if (param->output.length > 0 && param->output.data != NULL)
        {
            size_t dump_len = param->output.length > 16 ? 16 : param->output.length;
            ESP_LOG_BUFFER_HEX_LEVEL(TAG, param->output.data, dump_len, ESP_LOG_INFO);
        }
        break;

    case ESP_HIDD_FEATURE_EVENT:
        ESP_LOGI(TAG, "FEATURE trans=%u type=%u id=0x%02x len=%u map=%u",
                 (unsigned int)param->feature.trans_type,
                 (unsigned int)param->feature.report_type,
                 (unsigned int)param->feature.report_id,
                 (unsigned int)param->feature.length,
                 (unsigned int)param->feature.map_index);
        if (param->feature.length > 0 && param->feature.data != NULL)
        {
            size_t dump_len = param->feature.length > 16 ? 16 : param->feature.length;
            ESP_LOG_BUFFER_HEX_LEVEL(TAG, param->feature.data, dump_len, ESP_LOG_INFO);
        }
        if (param->feature.trans_type == ESP_HID_TRANS_GET_REPORT &&
            param->feature.report_type == ESP_HID_REPORT_TYPE_INPUT &&
            param->feature.report_id == 0x30 && s_hid_dev)
        {
            esp_err_t ret = esp_hidd_dev_input_set(s_hid_dev,
                                                   param->feature.map_index,
                                                   param->feature.report_id,
                                                   s_last_rpt_buf,
                                                   RPT_PAYLOAD);
            if (ret != ESP_OK)
            {
                ESP_LOGW(TAG, "GET_REPORT reply failed: %s", esp_err_to_name(ret));
            }
        }
        break;

    case ESP_HIDD_CONTROL_EVENT:
        ESP_LOGD(TAG, "CONTROL ctrl=%u map=%u",
                 (unsigned int)param->control.control,
                 (unsigned int)param->control.map_index);
        break;

    case ESP_HIDD_DISCONNECT_EVENT:
        ESP_LOGI(TAG, "HID disconnected: status=%s reason=0x%02x",
                 esp_err_to_name(param->disconnect.status),
                 (unsigned int)param->disconnect.reason);
        s_connected = false;
        s_force_active_acl_pending = false;
        if (s_connected_cb)
            s_connected_cb(false);
        bt_switch_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE,
                                "disconnect discoverable");
        break;

    case ESP_HIDD_STOP_EVENT:
        ESP_LOGI(TAG, "HID stop: %s", esp_err_to_name(param->stop.status));
        s_hid_dev = NULL;
        s_force_active_acl_pending = false;
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
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 5, 0)
    case ESP_BT_GAP_ACL_CONN_CMPL_STAT_EVT:
        ESP_LOGI(TAG, "ACL connect complete: stat=%d", (int)param->acl_conn_cmpl_stat.stat);
        break;

    case ESP_BT_GAP_ACL_DISCONN_CMPL_STAT_EVT:
        ESP_LOGI(TAG, "ACL disconnect complete: reason=0x%02x",
                 (unsigned int)param->acl_disconn_cmpl_stat.reason);
        break;
#endif

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

    case ESP_BT_GAP_CFM_REQ_EVT:
        ESP_LOGI(TAG,
                 "SSP confirm request from %02x:%02x:%02x:%02x:%02x:%02x num=%06u",
                 param->cfm_req.bda[0], param->cfm_req.bda[1], param->cfm_req.bda[2],
                 param->cfm_req.bda[3], param->cfm_req.bda[4], param->cfm_req.bda[5],
                 (unsigned int)param->cfm_req.num_val);
        esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
        break;

    case ESP_BT_GAP_KEY_NOTIF_EVT:
        ESP_LOGI(TAG, "SSP key notif: %06u", (unsigned int)param->key_notif.passkey);
        break;

    case ESP_BT_GAP_KEY_REQ_EVT:
        ESP_LOGI(TAG, "SSP key request");
        break;

    case ESP_BT_GAP_PIN_REQ_EVT:
    {
#if CONFIG_BT_SSP_ENABLED
        esp_bt_pin_code_t pin;
        memset(pin, 0, sizeof(pin));
        ESP_LOGW(TAG,
                 "Unexpected legacy PIN request from %02x:%02x:%02x:%02x:%02x:%02x while SSP is enabled",
                 param->pin_req.bda[0], param->pin_req.bda[1], param->pin_req.bda[2],
                 param->pin_req.bda[3], param->pin_req.bda[4], param->pin_req.bda[5]);
        esp_bt_gap_pin_reply(param->pin_req.bda, false, 0, pin);
#else
        esp_bt_pin_code_t pin;
        bt_switch_fill_pin_code(pin);
        ESP_LOGI(TAG, "PIN request from %02x:%02x:%02x:%02x:%02x:%02x",
                 param->pin_req.bda[0], param->pin_req.bda[1], param->pin_req.bda[2],
                 param->pin_req.bda[3], param->pin_req.bda[4], param->pin_req.bda[5]);
        esp_bt_gap_pin_reply(param->pin_req.bda, true, bt_switch_pin_length(), pin);
#endif
        break;
    }

    case ESP_BT_GAP_MODE_CHG_EVT:
        ESP_LOGI(TAG, "GAP mode change: mode=%d", (int)param->mode_chg.mode);
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 5, 0)
        if (s_force_active_acl_pending && param->mode_chg.mode == ESP_BT_PM_MD_SNIFF)
        {
            s_force_active_acl_pending = false;
            bt_switch_force_active_acl("mode-change sniff");
        }
#endif
        break;

    default:
        ESP_LOGD(TAG, "Unhandled GAP event: %d", (int)event);
        break;
    }
}

/* ---------- public API ---------------------------------------------------- */
void bt_switch_init(bt_switch_connected_cb_t on_connected)
{
    s_connected_cb = on_connected;
    s_connected = false;
    s_force_active_acl_pending = false;
    s_timer = 0;
    memset(s_last_rpt_buf, 0, sizeof(s_last_rpt_buf));
    s_hid_dev = NULL;

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    bt_cfg.mode = ESP_BT_MODE_CLASSIC_BT;
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT));

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 1, 0)
    esp_bluedroid_config_t bluedroid_cfg = BT_BLUEDROID_INIT_CONFIG_DEFAULT();
    bluedroid_cfg.ssp_en = CONFIG_BT_SSP_ENABLED;
    ESP_ERROR_CHECK(esp_bluedroid_init_with_cfg(&bluedroid_cfg));
#else
    ESP_ERROR_CHECK(esp_bluedroid_init());
#endif
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    ESP_ERROR_CHECK(esp_bt_gap_register_callback(bt_switch_gap_cb));
    ESP_ERROR_CHECK(esp_bt_dev_set_device_name("Pro Controller"));

#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 5, 0)
    bt_switch_register_device_id_record();
#endif

    /* Mirror HOJA's working COD exactly: Peripheral | Gamepad with service bits. */
    esp_bt_cod_t hid_cod;
    uint32_t cod_val = 0x002508;
    memcpy(&hid_cod, &cod_val, sizeof(uint32_t));
    ESP_ERROR_CHECK(esp_bt_gap_set_cod(hid_cod, ESP_BT_INIT_COD));

#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 1, 0)
#if CONFIG_BT_SSP_ENABLED
    esp_bt_sp_param_t param_type = ESP_BT_SP_IOCAP_MODE;
    esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_NONE;
    esp_err_t sec_ret = esp_bt_gap_set_security_param(param_type, &iocap, sizeof(iocap));
    if (sec_ret != ESP_OK)
    {
        ESP_LOGW(TAG, "set_security_param(IO_CAP_NONE) failed: %s", esp_err_to_name(sec_ret));
    }
#else
    esp_bt_pin_code_t pin_code;
    bt_switch_fill_pin_code(pin_code);
    esp_bt_pin_type_t pin_type = ESP_BT_PIN_TYPE_FIXED;
    ESP_ERROR_CHECK(esp_bt_gap_set_pin(pin_type, bt_switch_pin_length(), pin_code));
#endif
#else
#if !CONFIG_BT_SSP_ENABLED
    esp_bt_pin_code_t pin_code;
    bt_switch_fill_pin_code(pin_code);
    /* Legacy variable-PIN pairing (no SSP) */
    esp_bt_pin_type_t pin_type = ESP_BT_PIN_TYPE_VARIABLE;
    ESP_ERROR_CHECK(esp_bt_gap_set_pin(pin_type, 0, pin_code));
#endif
#endif

#if BEBOPCORE_BT_SDP_ENABLED
    ESP_ERROR_CHECK(esp_sdp_register_callback(bt_switch_sdp_cb));
    ESP_ERROR_CHECK(esp_sdp_init());
#endif

    ESP_ERROR_CHECK(esp_hidd_dev_init(&s_hid_config, ESP_HID_TRANSPORT_BT, bt_switch_hidd_cb, &s_hid_dev));
    ESP_LOGI(TAG, "bt_switch_init done");
}

void bt_switch_start(void)
{
    /* Advertising is set inside ESP_HIDD_START_EVENT; this is a no-op placeholder. */
    ESP_LOGI(TAG, "bt_switch_start (advertising set via START_EVENT)");
}

void bt_switch_stop(void)
{
    bt_switch_set_scan_mode(ESP_BT_NON_CONNECTABLE, ESP_BT_NON_DISCOVERABLE,
                            "stop hidden");
    if (s_hid_dev)
    {
        esp_err_t ret = esp_hidd_dev_deinit(s_hid_dev);
        if (ret != ESP_OK)
        {
            ESP_LOGW(TAG, "HID deinit failed: %s", esp_err_to_name(ret));
        }
        s_hid_dev = NULL;
    }
#if CONFIG_BT_SDP_COMMON_ENABLED
    esp_sdp_protocol_status_t sdp_status = {0};
    if (esp_sdp_get_protocol_status(&sdp_status) == ESP_OK && sdp_status.sdp_inited)
    {
        esp_sdp_deinit();
    }
#endif
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
    if (!s_connected || !s_hid_dev)
        return;
    uint8_t buf[RPT_PAYLOAD];
    pack_full_report(report, buf, s_timer++, 0x8 /* battery placeholder */);
    memcpy(s_last_rpt_buf, buf, RPT_PAYLOAD);
    esp_err_t ret = esp_hidd_dev_input_set(s_hid_dev, 0, 0x30, buf, RPT_PAYLOAD);
    if (ret != ESP_OK)
    {
        ESP_LOGD(TAG, "input report send failed: %s", esp_err_to_name(ret));
    }
}
