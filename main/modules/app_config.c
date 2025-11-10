#include <stdint.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "modules/app_config.h"

typedef struct {
    uint8_t id;
    uint8_t version;
    uint8_t camera_stream_status;
} app_config_t;

static app_config_t app_config;

void app_config_init()
{
    app_config.id = 0x11;
    app_config.version = 0;
    app_config.camera_stream_status = 10;
}

uint8_t app_get_id()
{
    return app_config.id;
}

uint8_t app_get_version()
{
    return app_config.version;
}

uint8_t app_get_camera_stream_status()
{
    return app_config.camera_stream_status;
}

uint8_t app_set_camera_stream_status(uint8_t value)
{
    if (value == 0 || value == 1 || app_config.camera_stream_status == value) {
        printf("set cam status wrong val\n");
        return 0;
    }
    
    printf("set cam status %u\n", value);
    app_config.camera_stream_status = value;

    return 1;
    
}