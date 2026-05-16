#include <string.h>

#include "esp_timer.h"

#include "input.h"
#include "input_gpio.h"
#include "input_matrix.h"

static bebopCORE_input_backend_t s_backend = BEBOPCORE_INPUT_BACKEND_GPIO;
static bebopCORE_input_state_t s_curr_state;
static bebopCORE_input_state_t s_prev_state;
static uint64_t s_hold_start_us[BEBOPCORE_BUTTON_COUNT];

static void input_backend_read(bebopCORE_input_state_t *state)
{
    if (s_backend == BEBOPCORE_INPUT_BACKEND_MATRIX)
    {
        input_matrix_read(state);
    }
    else
    {
        input_gpio_read(state);
    }
}

void input_init(const bebopCORE_input_config_t *config)
{
    bool active_low = true;

    if (config != NULL)
    {
        s_backend = config->backend;
        active_low = config->active_low;
    }

    memset(&s_curr_state, 0, sizeof(s_curr_state));
    memset(&s_prev_state, 0, sizeof(s_prev_state));
    memset(s_hold_start_us, 0, sizeof(s_hold_start_us));

    if (s_backend == BEBOPCORE_INPUT_BACKEND_MATRIX)
    {
        input_matrix_init(active_low);
    }
    else
    {
        input_gpio_init(active_low);
    }
}

void input_update(void)
{
    uint64_t now = (uint64_t)esp_timer_get_time();

    s_prev_state = s_curr_state;
    input_backend_read(&s_curr_state);

    for (int i = 0; i < BEBOPCORE_BUTTON_COUNT; i++)
    {
        if (!s_prev_state.buttons[i] && s_curr_state.buttons[i])
        {
            s_hold_start_us[i] = now;
        }
        else if (!s_curr_state.buttons[i])
        {
            s_hold_start_us[i] = 0;
        }
    }
}

const bebopCORE_input_state_t *input_get_state(void)
{
    return &s_curr_state;
}

bool input_was_pressed(bebopCORE_button_id_t button)
{
    return !s_prev_state.buttons[button] && s_curr_state.buttons[button];
}

bool input_is_pressed(bebopCORE_button_id_t button)
{
    return s_curr_state.buttons[button];
}

uint32_t input_get_hold_ms(bebopCORE_button_id_t button)
{
    if (!s_curr_state.buttons[button] || s_hold_start_us[button] == 0)
    {
        return 0;
    }

    uint64_t now = (uint64_t)esp_timer_get_time();
    return (uint32_t)((now - s_hold_start_us[button]) / 1000ULL);
}

bool input_is_held(bebopCORE_button_id_t button, uint32_t hold_ms)
{
    return input_get_hold_ms(button) >= hold_ms;
}

void input_build_report(bebopCORE_report_t *report, bebopCORE_protocol_t protocol, bool shift_active)
{
    const bebopCORE_input_state_t *state = &s_curr_state;
    (void)protocol;

    memset(report, 0, sizeof(*report));

    if (!shift_active)
    {
        report->button_a = state->buttons[BEBOPCORE_BUTTON_A];
        report->button_b = state->buttons[BEBOPCORE_BUTTON_B];
        report->button_x = state->buttons[BEBOPCORE_BUTTON_X];
        report->button_y = state->buttons[BEBOPCORE_BUTTON_Y];
        report->dpad_right = state->buttons[BEBOPCORE_BUTTON_DPAD_RIGHT];
        report->dpad_down = state->buttons[BEBOPCORE_BUTTON_DPAD_DOWN];
        report->dpad_left = state->buttons[BEBOPCORE_BUTTON_DPAD_LEFT];
        report->dpad_up = state->buttons[BEBOPCORE_BUTTON_DPAD_UP];
        report->trigger_l = state->buttons[BEBOPCORE_BUTTON_L];
        report->trigger_zl = state->buttons[BEBOPCORE_BUTTON_ZL];
        report->trigger_r = state->buttons[BEBOPCORE_BUTTON_R];
        report->trigger_zr = state->buttons[BEBOPCORE_BUTTON_ZR];
        report->button_start = state->buttons[BEBOPCORE_BUTTON_START];
        report->button_select = state->buttons[BEBOPCORE_BUTTON_SELECT];
        report->button_home = state->buttons[BEBOPCORE_BUTTON_HOME];
        report->button_capture = state->buttons[BEBOPCORE_BUTTON_CAPTURE];
    }
    else
    {
        report->button_x = state->buttons[BEBOPCORE_BUTTON_A];
        report->button_y = state->buttons[BEBOPCORE_BUTTON_B];
        report->trigger_zl = state->buttons[BEBOPCORE_BUTTON_L] || state->buttons[BEBOPCORE_BUTTON_ZL];
        report->trigger_zr = state->buttons[BEBOPCORE_BUTTON_R] || state->buttons[BEBOPCORE_BUTTON_ZR];
        report->button_home = state->buttons[BEBOPCORE_BUTTON_START] || state->buttons[BEBOPCORE_BUTTON_HOME];
        report->button_capture = state->buttons[BEBOPCORE_BUTTON_SELECT] || state->buttons[BEBOPCORE_BUTTON_CAPTURE];
        report->dpad_right = state->buttons[BEBOPCORE_BUTTON_DPAD_RIGHT];
        report->dpad_down = state->buttons[BEBOPCORE_BUTTON_DPAD_DOWN];
        report->dpad_left = state->buttons[BEBOPCORE_BUTTON_DPAD_LEFT];
        report->dpad_up = state->buttons[BEBOPCORE_BUTTON_DPAD_UP];
    }

    report->button_stick_left = state->buttons[BEBOPCORE_BUTTON_STICK_LEFT];
    report->button_stick_right = state->buttons[BEBOPCORE_BUTTON_STICK_RIGHT];
    report->lx = state->lx;
    report->ly = state->ly;
    report->rx = state->rx;
    report->ry = state->ry;
    report->lt = state->lt;
    report->rt = state->rt;
}
