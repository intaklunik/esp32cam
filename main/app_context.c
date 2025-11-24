#include <stdbool.h>
#include <esp_check.h>
#include <stdatomic.h>
#include "app_context.h"
#include "regs.h"
#include "bus/esp_i2c_slave.h"
#include "bus/esp_spi_slave.h"
#include "modules/app_config.h"
#include "modules/camera.h"

static const char * TAG = "AppContext";
static atomic_bool _app_status;

void app_shutdown()
{
    if (atomic_exchange(&_app_status, false) == true) {
        app_spi_disable();
        camera_free();
    }
}

bool app_status()
{
    return atomic_load(&_app_status);
}

esp_err_t init_app_context()
{
    esp_err_t ret = ESP_OK;

    atomic_init(&_app_status, true);
    app_config_init();
    ESP_RETURN_ON_ERROR(camera_init(), TAG, "camera_init failed");
    ESP_GOTO_ON_ERROR(i2c_init_slave(), err, TAG, "i2c_init_slave failed %s", esp_err_to_name(ret));
    ESP_GOTO_ON_ERROR(app_spi_slave_init(), err, TAG, "app_spi_slave_init failed %s", esp_err_to_name(ret));

    return ESP_OK;
err:
    app_shutdown();
    return ret;
}

/*
i2c context
*/

typedef enum {
    MODULE_APP = APP_BASE,
    MODULE_CAMERA = CAMERA_BASE
} module_t;

static regval_t i2c_tx_buffer = {
    .length = 0,
};

static inline void invalidate_i2c_tx_buffer()
{
    ESP_LOGD(TAG, "invalidate_i2c_tx_buffer");
    i2c_tx_buffer.length = 0;
}

static void i2c_app_handle(const cmd_t * cmd)
{
    ESP_LOGV(TAG, "i2c_app_handle: reg %x, length %d", cmd->reg, cmd->regval.length);
    if (cmd->regval.length == 0) {
        switch(cmd->reg) {
            case APP_ID_REG:
                i2c_tx_buffer.u8 = app_get_id();
                i2c_tx_buffer.length = 1;
                break;
            case APP_VERSION_REG:
                i2c_tx_buffer.u8 = app_get_version();
                i2c_tx_buffer.length = 1;
                break;
            case CAMERA_STREAM_STATUS_REG:
                i2c_tx_buffer.u8 = app_get_camera_stream_status();
                i2c_tx_buffer.length = 1;
                break;
            default:
                invalidate_i2c_tx_buffer();
                break;
        }
    }
    else {
        if (cmd->regval.length == 1) {
            switch(cmd->reg) {
                case CAMERA_STREAM_STATUS_REG:
                    uint8_t status_updated;
                    status_updated = app_set_camera_stream_status(cmd->regval.u8);
                    if (status_updated) {
                        ESP_LOGD(TAG, "camera_stream_status updated");
                        cmd->regval.u8 ? app_spi_enable() : app_spi_disable();
                    }
                    break;
            }
        }
    }

}

static void i2c_camera_handle(const cmd_t * cmd)
{
    ESP_LOGV(TAG, "i2c_camera_handle: reg %x, length %d", cmd->reg, cmd->regval.length);
    if (cmd->regval.length == 0) {
        switch(cmd->reg) {
            case CAM_BRIGHTNESS_REG:
                i2c_tx_buffer.u8 = camera_get_brightness();
                i2c_tx_buffer.length = 1;
                break;
            case CAM_CONTRAST_REG:
                i2c_tx_buffer.u8 = camera_get_contrast();
                i2c_tx_buffer.length = 1;
                break;
            case CAM_SATURATION_REG:
                i2c_tx_buffer.u8 = camera_get_saturation();
                i2c_tx_buffer.length = 1;
                break;
            default:
                invalidate_i2c_tx_buffer();
                break;
        }
    }
    else {
        if (cmd->regval.length == 1) {
            switch(cmd->reg) {
                case CAM_BRIGHTNESS_REG:
                    camera_set_brightness(cmd->regval.u8);
                    break;
                case CAM_CONTRAST_REG:
                    camera_set_contrast(cmd->regval.u8);
                    break;
                case CAM_SATURATION_REG:
                    camera_set_saturation(cmd->regval.u8);
                    break;
            }
        }
    }
}

void app_i2c_handle_rx(const cmd_t * cmd)
{
    module_t module_type = cmd->reg & 0xFF00;

    ESP_LOGD(TAG, "app_i2c_handle_rx, module_type: %x", module_type);

    switch(module_type) {
        case MODULE_APP:
            i2c_app_handle(cmd);
            break;
        case MODULE_CAMERA:
            i2c_camera_handle(cmd);
            break;
        default:
            if (cmd->regval.length > 0) {
                invalidate_i2c_tx_buffer();
            }
    }  
}

void app_i2c_prepare_tx(uint8_t ** data, uint8_t * length)
{
    *data = i2c_tx_buffer.buf;
    *length = i2c_tx_buffer.length;

    ESP_LOGD(TAG, "app_i2c_prepare_tx: length %d", *length);
}

/*
spi context
*/

void app_spi_prepare_tx(uint8_t ** data, uint32_t * length)
{
    *length = camera_get_fb(data);
}

void app_spi_free_tx()
{
    camera_free_fb();
}


