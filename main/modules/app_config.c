#include <stdint.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "esp_log.h"
#include "modules/app_config.h"

static const char * TAG = "AppModule";

typedef struct {
    const uint8_t id;
    const uint8_t version;
    uint8_t camera_stream_status;
} app_config_t;

static app_config_t app_config = {
    .id = 0x11,
    .version = 0,
    .camera_stream_status = 10,
};

void app_config_init()
{
    ESP_LOGV(TAG, "id: %x, version: %d, camera_stream_status: %d", app_config.id, app_config.version, app_config.camera_stream_status);
}

uint8_t app_get_id()
{
    ESP_LOGD(TAG, "get_id: %x", app_config.id);
    return app_config.id;
}

uint8_t app_get_version()
{
    ESP_LOGD(TAG, "get_version: %d", app_config.version);
    return app_config.version;
}

uint8_t app_get_camera_stream_status()
{
    ESP_LOGD(TAG, "get_camera_stream_status: %d", app_config.camera_stream_status);
    return app_config.camera_stream_status;
}

uint8_t app_set_camera_stream_status(uint8_t value)
{
    if (value == 0 || value == 1 || app_config.camera_stream_status == value) {
        ESP_LOGW(TAG, "app_set_camera_stream_status wrong value");
        return 0;
    }
    
    ESP_LOGD(TAG, "app_set_camera_stream_status, value: %u", value);
    app_config.camera_stream_status = value;

    return 1;
    
}