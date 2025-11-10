#include <esp_log.h>
#include <memory.h>
#include <driver/i2c_slave.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "bus/esp_i2c_slave.h"
#include "app_context.h"

#define I2C_MAX_EVENTS 20

#define I2C_SLAVE_RX_BUF_LEN 100
#define I2C_SLAVE_TX_BUF_LEN 100
#define I2C_REGISTER_SIZE 2

#define I2C_SLAVE_SCL_IO 2
#define I2C_SLAVE_SDA_IO 4

#define I2C_SLAVE_ADDR_APP 0x11

typedef cmd_t event_t;

typedef struct {
    QueueHandle_t event_queue;
    i2c_slave_dev_handle_t slave_handle;
} i2c_context_t;

static i2c_context_t IRAM_ATTR  context;

static const event_t tx_event;
static event_t rx_event;

static bool inline is_rx_event(const event_t * event)
{
    return event->reg;
}

static uint16_t inline to_le16(const uint8_t * buffer)
{
    return buffer[1] | (buffer[0] << 8);
} 

static bool IRAM_ATTR i2c_slave_request_cb(i2c_slave_dev_handle_t i2c_slave, const i2c_slave_request_event_data_t *evt_data, void *arg)
{
    BaseType_t xTaskWoken = pdFALSE;
    i2c_context_t * ctx = (i2c_context_t *)arg;
    
    xQueueSendFromISR(ctx->event_queue, &tx_event, &xTaskWoken);

    return xTaskWoken;
}

static bool IRAM_ATTR i2c_slave_receive_cb(i2c_slave_dev_handle_t i2c_slave, const i2c_slave_rx_done_event_data_t *evt_data, void *arg)
{
    BaseType_t xTaskWoken = pdFALSE;
    i2c_context_t * ctx = (i2c_context_t *)arg;
    
    if (evt_data->length >= I2C_REGISTER_SIZE) {
        rx_event.reg = to_le16(evt_data->buffer);
        rx_event.regval.length = evt_data->length - I2C_REGISTER_SIZE;
        
        if (rx_event.regval.length == 1) {
            rx_event.regval.u8 = evt_data->buffer[2];
        }
        else if (rx_event.regval.length == 2) {
            rx_event.regval.u16 = to_le16(&evt_data->buffer[2]);
        }

        xQueueSendFromISR(ctx->event_queue, &rx_event, &xTaskWoken);
    }
        
    return xTaskWoken;
}

static i2c_slave_event_callbacks_t cbs = {
    .on_request = NULL, //i2c_slave_request_cb,
    .on_receive = i2c_slave_receive_cb,
};

static void i2c_slave_task(void *arg) {
    i2c_context_t * context = arg;
    event_t event;
    uint8_t * tx_data;
    uint8_t tx_length = 0;
    uint32_t write_len;

    while (1) {
        if (xQueueReceive(context->event_queue, &event, pdMS_TO_TICKS(1000))) {
            printf("event: reg %x, len %u, regval %u\n", event.reg, event.regval.length, event.regval.u8);
            if(is_rx_event(&event)) {
                app_i2c_handle_rx(&event);
            //}
            //else {
                if (event.regval.length == 0) {
                    app_i2c_prepare_tx(&tx_data, &tx_length);
                    if (tx_length) {
                        ESP_ERROR_CHECK(i2c_slave_write(context->slave_handle, tx_data, tx_length, &write_len, 2000));
                        // printf("after write_len %lu\n", write_len);
                    }
                }
            }
        }
    }

    vTaskDelete(NULL);
}

esp_err_t i2c_init_slave()
{
    i2c_slave_config_t i2c_slave_config = {
        .i2c_port = -1,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .scl_io_num = I2C_SLAVE_SCL_IO,
        .sda_io_num = I2C_SLAVE_SDA_IO,
        .slave_addr = I2C_SLAVE_ADDR_APP,
        .send_buf_depth = I2C_SLAVE_RX_BUF_LEN,
        .receive_buf_depth = I2C_SLAVE_TX_BUF_LEN
    };

    context.event_queue = xQueueCreate(I2C_MAX_EVENTS, sizeof(event_t));
    if (!context.event_queue) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_ERROR_CHECK(i2c_new_slave_device(&i2c_slave_config, &context.slave_handle));
    ESP_ERROR_CHECK(i2c_slave_register_event_callbacks(context.slave_handle, &cbs, &context));

    xTaskCreate(i2c_slave_task, "i2c_slave_task", 4096, &context, 15, NULL);

    return ESP_OK;
}
