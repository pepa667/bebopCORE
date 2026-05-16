#include "storage.h"

#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "sdkconfig.h"

#define BEBOPCORE_STORAGE_NS "bebopcore"
#define BEBOPCORE_KEY_PROTOCOL "protocol"
#define BEBOPCORE_KEY_PAIRRST "pair_rst"

static const char *TAG = "bebopCORE_storage";
static bool s_storage_ready = false;

static bebopCORE_protocol_t storage_default_protocol(void)
{
#if CONFIG_BEBOPCORE_DEFAULT_PROTOCOL_XINPUT
    return BEBOPCORE_PROTOCOL_XINPUT;
#elif CONFIG_BEBOPCORE_DEFAULT_PROTOCOL_DINPUT
    return BEBOPCORE_PROTOCOL_DINPUT;
#elif CONFIG_BEBOPCORE_DEFAULT_PROTOCOL_GENERIC
    return BEBOPCORE_PROTOCOL_GENERIC;
#else
    return BEBOPCORE_PROTOCOL_SWITCH;
#endif
}

void storage_init(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    s_storage_ready = true;
}

bool storage_is_ready(void)
{
    return s_storage_ready;
}

void storage_save_active_protocol(bebopCORE_protocol_t protocol)
{
    if (!s_storage_ready)
    {
        return;
    }

    nvs_handle_t handle;
    if (nvs_open(BEBOPCORE_STORAGE_NS, NVS_READWRITE, &handle) != ESP_OK)
    {
        ESP_LOGW(TAG, "Failed to open NVS to save protocol");
        return;
    }

    nvs_set_u8(handle, BEBOPCORE_KEY_PROTOCOL, (uint8_t)protocol);
    nvs_commit(handle);
    nvs_close(handle);
}

bebopCORE_protocol_t storage_load_active_protocol(void)
{
    uint8_t value = (uint8_t)storage_default_protocol();
    if (!s_storage_ready)
    {
        return (bebopCORE_protocol_t)value;
    }

    nvs_handle_t handle;
    if (nvs_open(BEBOPCORE_STORAGE_NS, NVS_READONLY, &handle) != ESP_OK)
    {
        return (bebopCORE_protocol_t)value;
    }

    nvs_get_u8(handle, BEBOPCORE_KEY_PROTOCOL, &value);
    nvs_close(handle);

    if (value >= BEBOPCORE_PROTOCOL_COUNT)
    {
        value = (uint8_t)storage_default_protocol();
    }

    return (bebopCORE_protocol_t)value;
}

void storage_save_pairing_reset_counter(uint32_t value)
{
    if (!s_storage_ready)
    {
        return;
    }

    nvs_handle_t handle;
    if (nvs_open(BEBOPCORE_STORAGE_NS, NVS_READWRITE, &handle) != ESP_OK)
    {
        ESP_LOGW(TAG, "Failed to open NVS to save pairing reset counter");
        return;
    }

    nvs_set_u32(handle, BEBOPCORE_KEY_PAIRRST, value);
    nvs_commit(handle);
    nvs_close(handle);
}

uint32_t storage_load_pairing_reset_counter(void)
{
    uint32_t value = 0;
    if (!s_storage_ready)
    {
        return value;
    }

    nvs_handle_t handle;
    if (nvs_open(BEBOPCORE_STORAGE_NS, NVS_READONLY, &handle) != ESP_OK)
    {
        return value;
    }

    nvs_get_u32(handle, BEBOPCORE_KEY_PAIRRST, &value);
    nvs_close(handle);
    return value;
}
