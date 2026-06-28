/**
 * Copyright (c) HiSilicon (Shanghai) Technologies Co., Ltd. 2023-2023. All rights reserved.
 *
 * Description: SLE UART sample of client. \n
 *
 * History: \n
 * 2023-04-03, Create file. \n
 */
#ifndef SLE_UART_CLIENT_H
#define SLE_UART_CLIENT_H

#include <stdbool.h>
#include <stdint.h>

#include "sle_ssap_client.h"

void sle_uart_client_init(ssapc_notification_callback notification_cb, ssapc_indication_callback indication_cb);

void sle_uart_start_scan(void);

uint16_t get_g_sle_uart_conn_id(void);

ssapc_write_param_t *get_g_sle_uart_send_param(void);
bool sle_uart_client_can_send(void);
void sle_uart_client_mark_write_pending(void);
void sle_uart_client_clear_write_pending(void);
uint32_t sle_uart_client_get_write_cfm_count(void);
uint32_t sle_uart_client_get_write_cfm_fail_count(void);
void sle_uart_notification_cb(uint8_t client_id, uint16_t conn_id, ssapc_handle_value_t *data, errcode_t status);
void sle_uart_indication_cb(uint8_t client_id, uint16_t conn_id, ssapc_handle_value_t *data, errcode_t status);

#endif