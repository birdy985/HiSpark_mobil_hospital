#ifndef MAX30102_BSP_H
#define MAX30102_BSP_H

#include <stdint.h>
#include "errcode.h"

#define MAX30102_I2C_MASTER_ADDR      0x0
#define MAX30102_I2C_BAUDRATE         400000
#define CONFIG_I2C_MASTER_BUS_ID      1
/* HH-D01 header: TXD1/GPIO15 is I2C1_SDA, RXD1/GPIO16 is I2C1_SCL. */
#define CONFIG_I2C_SCL_MASTER_PIN     16
#define CONFIG_I2C_SDA_MASTER_PIN     15
#define CONFIG_I2C_MASTER_PIN_MODE    2

typedef struct {
    uint32_t red;
    uint32_t ir;
} max30102_sample_t;

errcode_t max30102_init(void);
errcode_t max30102_read_part_id(uint8_t *part_id);
errcode_t max30102_get_fifo_sample_count(uint8_t *sample_count);
errcode_t max30102_read_fifo_sample(max30102_sample_t *sample);

#endif