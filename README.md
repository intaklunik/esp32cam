# _ESP32CAM_

ESP32CAM project implements an ESP32-based camera system supporting video streaming and image capture. It provides 2 communication interfaces with the host device: I2C for configuration and SPI for streaming.

## Project structure
```
main
|
|   main.c - application entry point
|   app_context.h - interface between communication layer and application layer
|   app_context.c - connects communication layer (/bus) with application modules (/modules)
|   regs.h - defines device register addresses
|
|___bus
|   | esp_i2c_slave.h - public interface for I2C initialization
|   | esp_i2c_slave.c - implements the I2C slave driver
|   | esp_spi_slave.h - public interface for SPI setup and control
|   | esp_spi_slave.c - implements the SPI slave driver
|
|___modules
    | app_config.h - interface for accessing app config registers
    | app_config.c - implements read/write functions for config registers
    | camera.h - camera module interface
    | camera.c - implements camera driver and control logic

```

## Regmap
ESP32CAM application is organised in 2 logical modules. Each module has its own base address and every register inside the module is defined as:  
REGISTER_ADDRESS = MODULE_BASE | REGISTER_OFFSET

| Module | Base | Description |
|--------|------|-------------|
| APP | 0x0000 | general application configuration |
| CAMERA | 0x0100 | camera settings |

The table below describes the device registers and their access properties:
- Register - register name
- Address - register address
- Size - number of **bytes** used for reading/writing the register
- Read/Write - access permissions for the register: **R** - read-only, **RW** - read/write
- Value - allowed values/ranges for the register
- Description - short explanation of register purpose and behaviour

| Register                 | Address          | Size | Read/Write | Value | Description | 
|--------------------------|------------------|------|------------|-------|-------------|
| APP_ID_REG               | APP_BASE &#124; 0X01 | 1 | R | 0x11 | Application ID |
| APP_VERSION_REG          | APP_BASE &#124; 0X02 | 1 | R | 0 | Application version |
| CAMERA_STREAM_STATUS_REG | APP_BASE &#124; 0X03 | 1 | RW | TODO | Camera stream status, enables/disables SPI camera streaming |
| CAM_BRIGHTNESS_REG       | CAMERA_BASE &#124; 0X01 | 1 | RW | -2..2 | Camera brightness adjustment |
| CAM_CONTRAST_REG         | CAMERA_BASE &#124; 0X02 | 1 | RW | -2..2 | Camera contrast adjustment |
| CAM_SATURATION_REG       | CAMERA_BASE &#124; 0X03 | 1 | RW | -2..2 | Camera saturation adjustment |  

**Example:** address of *Camera contrast register* is: CAMERA_BASE | 0x02 = 0x0100 | 0x02 = 0x0102

## I2C
ESP32CAM registers an I2C slave interface for main communication with master device.
- SDA: GPIO 4
- SCL: GPIO 2  

I2C Address is **0x11**.

Protocol format: [ Register address ][ Value ]. The **MSB** is transmitted first.   
- Register address is always 2 bytes
- Value size is 1-2 bytes (register sizes are listed in the table above).

#### Read message  
1. From master to slave: [ MSB of register address ][ LSB of register address ]
2. From slave to master: [ MSB of data value ][ LSB of data value ]   

**Example:** to read *Application version register*, master sends a message: [0x00][0x02] and reads a message with the size of the register (1 byte): [0x0].
#### Write message
From master to slave: [ MSB of register address ][ LSB of register address ][ MSB of data value ][ LSB of data value ]    
**Example:** to write a value (1) to *Camera saturation register*, master sends a message: [0x01][0x03][0x01].

## Camera
Camera module uses the OV2640 sensor configured to capture frames at 120x160 resolution in RGB565 format.  
Supported settings (configured via I2C):
- brightness
- contrast
- saturation  

More information about camera in ESP-IDF projects: [https://github.com/espressif/esp32-camera](https://github.com/espressif/esp32-camera)