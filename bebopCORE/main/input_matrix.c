#include <string.h>

#include "driver/gpio.h"
#include "esp_rom_sys.h"
#include "esp_log.h"
#include "sdkconfig.h"

#include "input_matrix.h"

static const char *TAG = "bebopCORE_input_matrix";

static bool s_active_low = true;

static const gpio_num_t s_rows[4] = {
    (gpio_num_t)CONFIG_BEBOPCORE_MATRIX_ROW_0_GPIO,
    (gpio_num_t)CONFIG_BEBOPCORE_MATRIX_ROW_1_GPIO,
    (gpio_num_t)CONFIG_BEBOPCORE_MATRIX_ROW_2_GPIO,
    (gpio_num_t)CONFIG_BEBOPCORE_MATRIX_ROW_3_GPIO,
};

static const gpio_num_t s_cols[5] = {
    (gpio_num_t)CONFIG_BEBOPCORE_MATRIX_COL_0_GPIO,
    (gpio_num_t)CONFIG_BEBOPCORE_MATRIX_COL_1_GPIO,
    (gpio_num_t)CONFIG_BEBOPCORE_MATRIX_COL_2_GPIO,
    (gpio_num_t)CONFIG_BEBOPCORE_MATRIX_COL_3_GPIO,
    (gpio_num_t)CONFIG_BEBOPCORE_MATRIX_COL_4_GPIO,
};

static const bebopCORE_button_id_t s_button_map[4][5] = {
    {BEBOPCORE_BUTTON_A, BEBOPCORE_BUTTON_HOME, BEBOPCORE_BUTTON_L, BEBOPCORE_BUTTON_ZL, BEBOPCORE_BUTTON_SHIFT},
    {BEBOPCORE_BUTTON_B, BEBOPCORE_BUTTON_START, BEBOPCORE_BUTTON_R, BEBOPCORE_BUTTON_STICK_LEFT, BEBOPCORE_BUTTON_PROTOCOL},
    {BEBOPCORE_BUTTON_X, BEBOPCORE_BUTTON_SELECT, BEBOPCORE_BUTTON_DPAD_UP, BEBOPCORE_BUTTON_CAPTURE, BEBOPCORE_BUTTON_STICK_RIGHT},
    {BEBOPCORE_BUTTON_Y, BEBOPCORE_BUTTON_ZR, BEBOPCORE_BUTTON_DPAD_DOWN, BEBOPCORE_BUTTON_DPAD_LEFT, BEBOPCORE_BUTTON_DPAD_RIGHT},
};

void input_matrix_init(bool active_low)
{
    s_active_low = active_low;

    gpio_config_t row_cfg = {
        .pin_bit_mask = 0,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    for (int i = 0; i < 4; i++)
    {
        row_cfg.pin_bit_mask |= (1ULL << s_rows[i]);
    }
    gpio_config(&row_cfg);

    gpio_config_t col_cfg = {
        .pin_bit_mask = 0,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = active_low ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
        .pull_down_en = active_low ? GPIO_PULLDOWN_DISABLE : GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    for (int i = 0; i < 5; i++)
    {
        col_cfg.pin_bit_mask |= (1ULL << s_cols[i]);
    }
    gpio_config(&col_cfg);

    for (int i = 0; i < 4; i++)
    {
        gpio_set_level(s_rows[i], s_active_low ? 1 : 0);
    }

    ESP_LOGI(TAG, "Input matrix backend initialized");
}

void input_matrix_read(bebopCORE_input_state_t *state)
{
    memset(state, 0, sizeof(*state));

    for (int row = 0; row < 4; row++)
    {
        for (int i = 0; i < 4; i++)
        {
            gpio_set_level(s_rows[i], s_active_low ? 1 : 0);
        }

        gpio_set_level(s_rows[row], s_active_low ? 0 : 1);
        esp_rom_delay_us(50);

        for (int col = 0; col < 5; col++)
        {
            int level = gpio_get_level(s_cols[col]);
            bool pressed = s_active_low ? (level == 0) : (level != 0);
            state->buttons[s_button_map[row][col]] = pressed;
        }
    }
}
