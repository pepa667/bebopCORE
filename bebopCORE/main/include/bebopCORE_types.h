#ifndef BEBOPCORE_TYPES_H
#define BEBOPCORE_TYPES_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    BEBOPCORE_BUTTON_A = 0,
    BEBOPCORE_BUTTON_B,
    BEBOPCORE_BUTTON_X,
    BEBOPCORE_BUTTON_Y,
    BEBOPCORE_BUTTON_DPAD_RIGHT,
    BEBOPCORE_BUTTON_DPAD_DOWN,
    BEBOPCORE_BUTTON_DPAD_LEFT,
    BEBOPCORE_BUTTON_DPAD_UP,
    BEBOPCORE_BUTTON_L,
    BEBOPCORE_BUTTON_ZL,
    BEBOPCORE_BUTTON_R,
    BEBOPCORE_BUTTON_ZR,
    BEBOPCORE_BUTTON_START,
    BEBOPCORE_BUTTON_SELECT,
    BEBOPCORE_BUTTON_HOME,
    BEBOPCORE_BUTTON_CAPTURE,
    BEBOPCORE_BUTTON_STICK_LEFT,
    BEBOPCORE_BUTTON_STICK_RIGHT,
    BEBOPCORE_BUTTON_SHIFT,
    BEBOPCORE_BUTTON_PROTOCOL,
    BEBOPCORE_BUTTON_COUNT
} bebopCORE_button_id_t;

typedef enum {
    BEBOPCORE_PROTOCOL_SWITCH = 0,
    BEBOPCORE_PROTOCOL_XINPUT,
    BEBOPCORE_PROTOCOL_DINPUT,
    BEBOPCORE_PROTOCOL_GENERIC,
    BEBOPCORE_PROTOCOL_COUNT
} bebopCORE_protocol_t;

typedef enum {
    BEBOPCORE_CONN_IDLE = 0,
    BEBOPCORE_CONN_SEARCHING,
    BEBOPCORE_CONN_PAIRING,
    BEBOPCORE_CONN_CONNECTED
} bebopCORE_connection_state_t;

typedef struct {
    bool buttons[BEBOPCORE_BUTTON_COUNT];
    int16_t lx;
    int16_t ly;
    int16_t rx;
    int16_t ry;
    uint16_t lt;
    uint16_t rt;
} bebopCORE_input_state_t;

typedef struct {
    bool button_a;
    bool button_b;
    bool button_x;
    bool button_y;

    bool dpad_right;
    bool dpad_down;
    bool dpad_left;
    bool dpad_up;

    bool trigger_l;
    bool trigger_zl;
    bool trigger_r;
    bool trigger_zr;

    bool button_start;
    bool button_select;
    bool button_home;
    bool button_capture;

    bool button_stick_left;
    bool button_stick_right;

    int16_t lx;
    int16_t ly;
    int16_t rx;
    int16_t ry;
    uint16_t lt;
    uint16_t rt;
} bebopCORE_report_t;

#endif
