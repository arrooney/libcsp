/*
 * Copyright (C) 2021  University of Alberta
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
/**
 * @file service_utilities.c
 * @author Andrew Rooney
 * @date 2020-07-23
 */
#include <FreeRTOS.h>
#include <os_semphr.h>

#include <csp/csp.h>
#include <csp/drivers/usart.h>
#include <csp/arch/csp_malloc.h>
#include <csp/arch/csp_thread.h>

#include "HL_sci.h"
#include "HL_sys_common.h"
#include "HL_system.h"

typedef struct {
    csp_usart_callback_t rx_callback;
    void * user_data;
    sciBASE_t *fd;
    xTaskHandle rx_thread;
} usart_context_t;

xQueueHandle sciData;
uint8_t incomingData;

void usart_rx_thread(void * arg) {

    usart_context_t * ctx = arg;
    uint8_t rxByte;

    // Receive loop
    while (1) {
        if (xQueueReceive(sciData, &rxByte, portMAX_DELAY)){
            ctx->rx_callback(ctx->user_data, &rxByte, sizeof(uint8_t), NULL);
        }
    }
}

int csp_usart_write(sciBASE_t *fd, const void * data, size_t data_length) {
    uint8_t i = 0;
    for (i = 0; i < data_length; i++) {
        sciSend(sciREG3, 1, (uint8_t*)data + i);
    }

    return CSP_ERR_NONE;

}

int csp_usart_open(const csp_usart_conf_t *conf, csp_usart_callback_t rx_callback, void * user_data, sciBASE_t * return_fd) {
    _enable_IRQ();
    sciInit();
    sciSetBaudrate(sciREG3, conf->baudrate);

    usart_context_t * ctx = csp_calloc(1, sizeof(*ctx));

    ctx->rx_callback = rx_callback;
    ctx->user_data = user_data;
    ctx->fd = sciREG3;
    return_fd = sciREG3;
    sciData = xQueueCreate((unsigned portBASE_TYPE)32,
            (unsigned portBASE_TYPE)sizeof(uint8_t));
    xTaskCreate(usart_rx_thread, "usart_rx", 256, (void *) ctx, configMAX_PRIORITIES, &ctx->rx_thread);

    sciReceive(ctx->fd, sizeof(uint8_t), &incomingData);

    return CSP_ERR_NONE;
}


void csp_sciNotification(sciBASE_t *sci, unsigned flags) {
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendToBackFromISR( sciData, &incomingData, &xHigherPriorityTaskWoken );
    sciReceive(sci, sizeof(uint8_t), &incomingData);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    return;
}

void esmGroup1Notification(int but) {
    return;
}

void esmGroup2Notification(int but) {
    return;
}
