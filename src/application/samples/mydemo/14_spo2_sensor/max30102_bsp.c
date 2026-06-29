#include "max30102_bsp.h"
#include "max30102_reg.h"

#include <stdio.h>
#include "common_def.h"
#include "gpio.h"
#include "i2c.h"
#include "pinctrl.h"
#include "soc_osal.h"

#define MAX30102_RESET_DELAY_MS       100

static void max30102_i2c_init_pin(void)
{
    uapi_pin_set_mode(CONFIG_I2C_SCL_MASTER_PIN, CONFIG_I2C_MASTER_PIN_MODE);
    uapi_pin_set_mode(CONFIG_I2C_SDA_MASTER_PIN, CONFIG_I2C_MASTER_PIN_MODE);
    uapi_pin_set_pull(CONFIG_I2C_SCL_MASTER_PIN, PIN_PULL_TYPE_STRONG_UP);
    uapi_pin_set_pull(CONFIG_I2C_SDA_MASTER_PIN, PIN_PULL_TYPE_STRONG_UP);
    uapi_gpio_set_val(CONFIG_I2C_SCL_MASTER_PIN, GPIO_LEVEL_HIGH);
    uapi_gpio_set_val(CONFIG_I2C_SDA_MASTER_PIN, GPIO_LEVEL_HIGH);
}

static errcode_t max30102_write_reg(uint8_t reg, uint8_t value)
{
    uint8_t buffer[2] = {reg, value};
    i2c_data_t data = {
        .send_buf = buffer,
        .send_len = sizeof(buffer),
    };

    return uapi_i2c_master_write(CONFIG_I2C_MASTER_BUS_ID, MAX30102_I2C_ADDR, &data);
}

static errcode_t max30102_read_bytes(uint8_t reg, uint8_t *buffer, uint32_t len)
{
    i2c_data_t data = {
        .send_buf = &reg,
        .send_len = 1,
        .receive_buf = buffer,
        .receive_len = len,
    };

    return uapi_i2c_master_writeread(CONFIG_I2C_MASTER_BUS_ID, MAX30102_I2C_ADDR, &data);
}

static errcode_t max30102_read_reg(uint8_t reg, uint8_t *value)
{
    return max30102_read_bytes(reg, value, 1);
}

static errcode_t max30102_clear_fifo(void)
{
    errcode_t ret;

    ret = max30102_write_reg(MAX30102_REG_FIFO_WR_PTR, 0);
    if (ret != ERRCODE_SUCC) {
        return ret;
    }

    ret = max30102_write_reg(MAX30102_REG_OVF_COUNTER, 0);
    if (ret != ERRCODE_SUCC) {
        return ret;
    }

    return max30102_write_reg(MAX30102_REG_FIFO_RD_PTR, 0);
}

errcode_t max30102_read_part_id(uint8_t *part_id)
{
    if (part_id == NULL) {
        return ERRCODE_INVALID_PARAM;
    }

    return max30102_read_reg(MAX30102_REG_PART_ID, part_id);
}

errcode_t max30102_init(void)
{
    errcode_t ret;
    uint8_t part_id = 0;

    max30102_i2c_init_pin();
    ret = uapi_i2c_master_init(CONFIG_I2C_MASTER_BUS_ID, MAX30102_I2C_BAUDRATE, MAX30102_I2C_MASTER_ADDR);
    if (ret != ERRCODE_SUCC) {
        printf("[SPO2] i2c init failed, ret=0x%x\r\n", (unsigned int)ret);
        return ret;
    }

    ret = max30102_read_part_id(&part_id);
    if (ret != ERRCODE_SUCC) {
        printf("[SPO2] part id read failed, ret=0x%x\r\n", (unsigned int)ret);
        return ret;
    }
    if (part_id != MAX30102_EXPECTED_PART_ID) {
        printf("[SPO2] unexpected PART_ID=0x%02x, expected=0x%02x\r\n",
            part_id, MAX30102_EXPECTED_PART_ID);
        return ERRCODE_FAIL;
    }

    ret = max30102_write_reg(MAX30102_REG_MODE_CONFIG, MAX30102_MODE_RESET);
    if (ret != ERRCODE_SUCC) {
        return ret;
    }
    osal_msleep(MAX30102_RESET_DELAY_MS);

    ret = max30102_clear_fifo();
    if (ret != ERRCODE_SUCC) {
        return ret;
    }

    ret = max30102_write_reg(MAX30102_REG_INTR_ENABLE_1, 0);
    if (ret != ERRCODE_SUCC) {
        return ret;
    }
    ret = max30102_write_reg(MAX30102_REG_INTR_ENABLE_2, 0);
    if (ret != ERRCODE_SUCC) {
        return ret;
    }

    ret = max30102_write_reg(MAX30102_REG_FIFO_CONFIG,
        MAX30102_FIFO_AVG_1 | MAX30102_FIFO_ROLLOVER_EN | MAX30102_FIFO_A_FULL_15);
    if (ret != ERRCODE_SUCC) {
        return ret;
    }

    ret = max30102_write_reg(MAX30102_REG_SPO2_CONFIG,
        MAX30102_SPO2_ADC_RGE_4096NA | MAX30102_SPO2_SR_100HZ | MAX30102_LED_PW_411US_18BIT);
    if (ret != ERRCODE_SUCC) {
        return ret;
    }

    ret = max30102_write_reg(MAX30102_REG_LED1_PA, MAX30102_LED_PA_DEFAULT);
    if (ret != ERRCODE_SUCC) {
        return ret;
    }
    ret = max30102_write_reg(MAX30102_REG_LED2_PA, MAX30102_LED_PA_DEFAULT);
    if (ret != ERRCODE_SUCC) {
        return ret;
    }

    ret = max30102_write_reg(MAX30102_REG_MODE_CONFIG, MAX30102_MODE_SPO2);
    if (ret != ERRCODE_SUCC) {
        return ret;
    }

    (void)max30102_read_reg(MAX30102_REG_INTR_STATUS_1, &part_id);
    (void)max30102_read_reg(MAX30102_REG_INTR_STATUS_2, &part_id);

    printf("[SPO2] MAX30102 ready, PART_ID=0x%02x\r\n", MAX30102_EXPECTED_PART_ID);
    return ERRCODE_SUCC;
}

errcode_t max30102_get_fifo_sample_count(uint8_t *sample_count)
{
    errcode_t ret;
    uint8_t write_ptr = 0;
    uint8_t read_ptr = 0;

    if (sample_count == NULL) {
        return ERRCODE_INVALID_PARAM;
    }

    ret = max30102_read_reg(MAX30102_REG_FIFO_WR_PTR, &write_ptr);
    if (ret != ERRCODE_SUCC) {
        return ret;
    }
    ret = max30102_read_reg(MAX30102_REG_FIFO_RD_PTR, &read_ptr);
    if (ret != ERRCODE_SUCC) {
        return ret;
    }

    write_ptr &= MAX30102_FIFO_PTR_MASK;
    read_ptr &= MAX30102_FIFO_PTR_MASK;
    if (write_ptr >= read_ptr) {
        *sample_count = write_ptr - read_ptr;
    } else {
        *sample_count = (MAX30102_FIFO_DEPTH - read_ptr) + write_ptr;
    }

    return ERRCODE_SUCC;
}

errcode_t max30102_read_fifo_sample(max30102_sample_t *sample)
{
    uint8_t raw[MAX30102_FIFO_FRAME_SIZE] = {0};
    errcode_t ret;

    if (sample == NULL) {
        return ERRCODE_INVALID_PARAM;
    }

    ret = max30102_read_bytes(MAX30102_REG_FIFO_DATA, raw, sizeof(raw));
    if (ret != ERRCODE_SUCC) {
        return ret;
    }

    sample->red = (((uint32_t)raw[0] << 16) | ((uint32_t)raw[1] << 8) | raw[2]) & MAX30102_SAMPLE_MASK;
    sample->ir = (((uint32_t)raw[3] << 16) | ((uint32_t)raw[4] << 8) | raw[5]) & MAX30102_SAMPLE_MASK;

    return ERRCODE_SUCC;
}
