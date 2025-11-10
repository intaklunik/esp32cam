#include <stdint.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <driver/gpio.h>
#include <driver/spi_slave.h>
#include "esp_log.h"
#include "bus/esp_spi_slave.h"
#include "app_context.h"

#define RCV_HOST SPI2_HOST
#define GPIO_MOSI 12
#define GPIO_MISO 13
#define GPIO_SCLK 15
#define GPIO_CS 14

static const char * spi_task_name = "spi_slave_task";
static const char * TAG = "SPI";

typedef struct {
    TaskHandle_t xHandle;
    bool enabled;
} spi_context_t;

static spi_context_t context;

static void spi_slave_task(void * arg);

esp_err_t app_spi_slave_init()
{
    spi_bus_config_t bus_config = {
        .mosi_io_num = GPIO_MOSI,
        .miso_io_num = GPIO_MISO,
        .sclk_io_num = GPIO_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    spi_slave_interface_config_t slave_config = {
        .mode = 0,
        .spics_io_num = GPIO_CS,
        .queue_size = 2,
        .flags = 0,
        .post_setup_cb = NULL,
        .post_trans_cb = NULL
    };

    esp_err_t ret = ESP_OK;
    
    gpio_set_pull_mode(bus_config.mosi_io_num, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(bus_config.sclk_io_num, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(slave_config.spics_io_num, GPIO_PULLUP_ONLY);

    ret = spi_slave_initialize(RCV_HOST, &bus_config, &slave_config, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "spi_slave_initialize failed: %s", esp_err_to_name(ret));
        return ret;
    }
    context.enabled = false;
    ret = app_spi_update();

    return ret;
}

esp_err_t app_spi_update()
{
    esp_err_t ret = ESP_OK;

    context.enabled = !context.enabled;

    if (context.enabled) {
        xTaskCreate(spi_slave_task, spi_task_name, 4096, &context, 10, &context.xHandle);
        if (!context.xHandle) {
            ESP_LOGE(TAG, "xTaskCreate(%s) failed", spi_task_name);
            context.enabled = !context.enabled;
            return ESP_ERR_NO_MEM;
        }
    }

    return ret;
}

static void spi_slave_task(void * arg) 
{
    ESP_LOGV(TAG, "%s start", spi_task_name);

    esp_err_t ret = ESP_OK;
    spi_context_t * context = arg;   
    uint8_t * tx_buffer;
    uint32_t tx_length;
    spi_slave_transaction_t transaction = {
        .flags = 0,
        .rx_buffer = NULL,
    };

    while (context->enabled) {
        app_spi_prepare_tx(&tx_buffer, &tx_length);
        transaction.length = tx_length * 8;
        transaction.tx_buffer = tx_buffer;
        ret = spi_slave_transmit(RCV_HOST, &transaction, portMAX_DELAY);
        app_spi_free_tx();

        ESP_LOGD(TAG, "spi_slave_transmit returned %s", esp_err_to_name(ret));
    }
    
    vTaskDelete(NULL);
}


