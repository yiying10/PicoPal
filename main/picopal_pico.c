#include "picopal_pico.h"

#include <stddef.h>

#include "picopal_framebuffer.h"
#include "picopal_graphics.h"

#define EYE(x_value, y_value, width_value, height_value, radius_value) \
    {                                                                  \
        .x = (x_value),                                                 \
        .y = (y_value),                                                 \
        .width = (width_value),                                         \
        .height = (height_value),                                       \
        .radius = (radius_value),                                       \
    }

static const picopal_pico_geometry_t s_poses[PICOPAL_PICO_POSE_COUNT] = {
    [PICOPAL_PICO_POSE_NEUTRAL] = {
        .left = EYE(18, 12, 36, 40, 10),
        .right = EYE(74, 12, 36, 40, 10),
    },
    [PICOPAL_PICO_POSE_LOOK_LEFT] = {
        .left = EYE(13, 12, 36, 40, 10),
        .right = EYE(69, 12, 36, 40, 10),
    },
    [PICOPAL_PICO_POSE_LOOK_RIGHT] = {
        .left = EYE(23, 12, 36, 40, 10),
        .right = EYE(79, 12, 36, 40, 10),
    },
    [PICOPAL_PICO_POSE_LOOK_UP] = {
        .left = EYE(18, 7, 36, 40, 10),
        .right = EYE(74, 7, 36, 40, 10),
    },
    [PICOPAL_PICO_POSE_LOOK_DOWN] = {
        .left = EYE(18, 17, 36, 40, 10),
        .right = EYE(74, 17, 36, 40, 10),
    },
    [PICOPAL_PICO_POSE_FOCUSED] = {
        .left = EYE(16, 18, 40, 28, 8),
        .right = EYE(72, 18, 40, 28, 8),
    },
    [PICOPAL_PICO_POSE_HAPPY] = {
        .left = EYE(17, 12, 38, 38, 12),
        .right = EYE(73, 12, 38, 38, 12),
        .style = PICOPAL_PICO_STYLE_HAPPY,
    },
    [PICOPAL_PICO_POSE_SLEEPY] = {
        .left = EYE(17, 29, 38, 6, 3),
        .right = EYE(73, 29, 38, 6, 3),
    },
    [PICOPAL_PICO_POSE_SURPRISED] = {
        .left = EYE(13, 8, 42, 48, 15),
        .right = EYE(73, 8, 42, 48, 15),
    },
    [PICOPAL_PICO_POSE_ANGRY] = {
        .left = EYE(17, 18, 38, 28, 7),
        .right = EYE(73, 18, 38, 28, 7),
        .style = PICOPAL_PICO_STYLE_ANGRY,
    },
};

static void render_eye(const picopal_eye_geometry_t *eye)
{
    picopal_graphics_fill_rounded_rect(
        eye->x,
        eye->y,
        eye->width,
        eye->height,
        eye->radius,
        true
    );
}

static void apply_happy_mask(const picopal_eye_geometry_t *eye)
{
    picopal_graphics_fill_rect(
        eye->x,
        eye->y + eye->height / 2,
        eye->width,
        eye->height / 2,
        false
    );
}

static void apply_angry_mask(
    const picopal_eye_geometry_t *eye,
    bool left_eye
)
{
    for (int16_t column = 0; column < eye->width; ++column) {
        int16_t depth = left_eye
            ? (eye->width - 1 - column) / 4
            : column / 4;
        picopal_graphics_fill_rect(
            eye->x + column,
            eye->y,
            1,
            depth,
            false
        );
    }
}

const picopal_pico_geometry_t *picopal_pico_pose_geometry(
    picopal_pico_pose_t pose
)
{
    if (pose < 0 || pose >= PICOPAL_PICO_POSE_COUNT) {
        pose = PICOPAL_PICO_POSE_NEUTRAL;
    }
    return &s_poses[pose];
}

void picopal_pico_render_geometry(const picopal_pico_geometry_t *geometry)
{
    if (geometry == NULL) {
        geometry = &s_poses[PICOPAL_PICO_POSE_NEUTRAL];
    }

    picopal_framebuffer_clear(false);
    render_eye(&geometry->left);
    render_eye(&geometry->right);

    if (geometry->style == PICOPAL_PICO_STYLE_HAPPY) {
        apply_happy_mask(&geometry->left);
        apply_happy_mask(&geometry->right);
    } else if (geometry->style == PICOPAL_PICO_STYLE_ANGRY) {
        apply_angry_mask(&geometry->left, true);
        apply_angry_mask(&geometry->right, false);
    }
}

void picopal_pico_render(picopal_pico_pose_t pose)
{
    picopal_pico_render_geometry(picopal_pico_pose_geometry(pose));
}
