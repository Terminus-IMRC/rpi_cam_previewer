/*
 * Copyright (c) 2017 Sugizaki Yukimasa (ysugi@idein.jp)
 * All rights reserved.
 *
 * This software is licensed under a Modified (3-Clause) BSD License.
 * You should have received a copy of this license along with this
 * software. If not, contact the copyright holder above.
 */

#include <interface/mmal/mmal.h>
#include <interface/mmal/mmal_logging.h>
#include <interface/mmal/util/mmal_connection.h>
#include <interface/mmal/util/mmal_util_params.h>
#include <interface/mmal/util/mmal_component_wrapper.h>
#include <interface/mmal/util/mmal_default_components.h>
#include <bcm_host.h>
#include <stdio.h>
#include <stdlib.h>


#define _STR(x) #x
#define STR(x) _STR(x)

#define _check(x) do { \
    MMAL_STATUS_T status = (x); \
    if (status != MMAL_SUCCESS) { \
        vcos_log_error("%s:%d: Assertation failed: %d\n", __FILE__, __LINE__, status); \
        exit(EXIT_FAILURE); \
    } \
} while (0)


#define CAMERA_NUM 0

static MMAL_RECT_T preview_dest_rect = {
    .x = 1024 / 4,
    .y = 768 / 4,
    .width = 1024 / 2,
    .height = 768 / 2
};
static MMAL_PARAMETER_FPS_RANGE_T preview_fps_range = {
    {MMAL_PARAMETER_FPS_RANGE, sizeof(preview_fps_range)},
    /* These are rational numbers ($1/$2). */
    .fps_low =  {1, 1},
    .fps_high = {3, 1}
};
static const int preview_width = 1024;
static const int preview_height = 768;


int main(int argc, char *argv[])
{
    MMAL_WRAPPER_T *cp_camera = NULL, *cp_video_renderer = NULL;
    MMAL_CONNECTION_T *connection = NULL;

    (void) argc;

    bcm_host_init();
    vcos_log_register(argv[0], VCOS_LOG_CATEGORY);

    /* Create camera and video_render component. */
    _check(mmal_wrapper_create(&cp_camera, MMAL_COMPONENT_DEFAULT_CAMERA));
    _check(mmal_wrapper_create(&cp_video_renderer, MMAL_COMPONENT_DEFAULT_VIDEO_RENDERER));

    {
        /* Control port of camera. */
        MMAL_PORT_T *control = cp_camera->control;

        /* Set camera number. */
        MMAL_PARAMETER_INT32_T camera_num = {
            {MMAL_PARAMETER_CAMERA_NUM, sizeof(camera_num)},
            CAMERA_NUM
        };
        _check(mmal_port_parameter_set(control, &camera_num.hdr));
    }

    {
        /* Preview port of camera. */
        MMAL_PORT_T *output = cp_camera->output[0];

        /* Set port format. */
        output->format->encoding = MMAL_ENCODING_OPAQUE;
        output->format->es->video.width = VCOS_ALIGN_UP(preview_width, 32);
        output->format->es->video.height = VCOS_ALIGN_UP(preview_height, 16);
        output->format->es->video.crop.x = 0;
        output->format->es->video.crop.y = 0;
        output->format->es->video.crop.width = preview_width;
        output->format->es->video.crop.height = preview_height;
        output->format->es->video.frame_rate.num = 0;
        output->format->es->video.frame_rate.den = 1;
        _check(mmal_port_format_commit(output));

        /* Set fps. */
        _check(mmal_port_parameter_set(output, &preview_fps_range.hdr));
    }

    {
        /* Input port of video_renderer. */
        MMAL_PORT_T *input = cp_video_renderer->input[0];

        MMAL_DISPLAYREGION_T displayregion = {
            {MMAL_PARAMETER_DISPLAYREGION, sizeof(displayregion)},
            .set = MMAL_DISPLAY_SET_DEST_RECT | MMAL_DISPLAY_SET_FULLSCREEN,
            .fullscreen = 0,
            .dest_rect = preview_dest_rect
        };
        _check(mmal_port_parameter_set(input, &displayregion.hdr));
    }

    /* Create connection between camera's preview and video_renderer. */
    _check(mmal_connection_create(
        &connection,
        cp_camera->output[0], cp_video_renderer->input[0],
        MMAL_CONNECTION_FLAG_TUNNELLING | MMAL_CONNECTION_FLAG_ALLOCATION_ON_INPUT
    ));
    /* And enable it. */
    _check(mmal_connection_enable(connection));

    /* Preview for 7 seconds. */
    vcos_sleep(7000);

    _check(mmal_connection_destroy(connection));
    _check(mmal_wrapper_destroy(cp_video_renderer));
    _check(mmal_wrapper_destroy(cp_camera));
    bcm_host_deinit();
    return 0;
}
