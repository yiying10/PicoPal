#pragma once

#include <stdint.h>

typedef enum {
    PICOPAL_PICO_STYLE_STANDARD,
    PICOPAL_PICO_STYLE_HAPPY,
    PICOPAL_PICO_STYLE_ANGRY,
} picopal_pico_style_t;

typedef struct {
    int16_t x;
    int16_t y;
    int16_t width;
    int16_t height;
    int16_t radius;
} picopal_eye_geometry_t;

typedef struct {
    picopal_eye_geometry_t left;
    picopal_eye_geometry_t right;
    picopal_pico_style_t style;
} picopal_pico_geometry_t;

typedef enum {
    PICOPAL_PICO_POSE_NEUTRAL,
    PICOPAL_PICO_POSE_LOOK_LEFT,
    PICOPAL_PICO_POSE_LOOK_RIGHT,
    PICOPAL_PICO_POSE_LOOK_UP,
    PICOPAL_PICO_POSE_LOOK_DOWN,
    PICOPAL_PICO_POSE_FOCUSED,
    PICOPAL_PICO_POSE_HAPPY,
    PICOPAL_PICO_POSE_SLEEPY,
    PICOPAL_PICO_POSE_SURPRISED,
    PICOPAL_PICO_POSE_ANGRY,
    PICOPAL_PICO_POSE_DIZZY,
    PICOPAL_PICO_POSE_COUNT,
} picopal_pico_pose_t;

const picopal_pico_geometry_t *picopal_pico_pose_geometry(
    picopal_pico_pose_t pose
);
void picopal_pico_render_geometry(const picopal_pico_geometry_t *geometry);
void picopal_pico_render(picopal_pico_pose_t pose);
