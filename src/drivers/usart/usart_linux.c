/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2012 GomSpace ApS (http://www.gomspace.com)
Copyright (C) 2012 AAUSAT3 Project (http://aausat3.space.aau.dk)

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "HL_sci.h"
#include "HL_sys_common.h"
#include "HL_system.h"
#include <csp/drivers/usart.h>
#include "FreeRTOS.h"
#include "os_semphr.h"
#include <stdio.h>

#include <csp/csp.h>
#include <csp/arch/csp_malloc.h>
#include <csp/arch/csp_thread.h>

typedef struct {
	csp_usart_callback_t rx_callback;
	void * user_data;
	sciBASE_t fd;
	xTaskHandle rx_thread;
} usart_context_t;

static int openPort(const char * device, sciBASE_t * return_fd) {

    return_fd = sciREG3;

    return CSP_ERR_NONE;
}

static void * usart_rx_thread(void * arg) {

	usart_context_t * ctx = arg;
	const unsigned int CBUF_SIZE = 400;
	uint8_t * cbuf = malloc(CBUF_SIZE);
	uint8_t rxByte;

	// Receive loop
	while (1) {
	    if (sciIsRxReady(ctx->fd)) {
	        rxByte = sciReceiveByte(ctx->fd);
	        ctx->rx_callback(ctx->user_data, rxByte, sizeof(uint8_t), NULL);
	    }
//		int length = read(ctx->fd, cbuf, CBUF_SIZE);
//		if (length <= 0) {
//			csp_log_error("%s: read() failed, returned: %d", __FUNCTION__, length);
//			exit(1);
//		}
//        ctx->rx_callback(ctx->user_data, cbuf, length, NULL);
	}
	return NULL;
}

int csp_usart_write(sciBASE_t fd, const void * data, size_t data_length) {
    int i = 0;
    for (i; i < data_length; i++) {
        while (!sciIsTxReady(fd));
        sciSendByte(fd, data + i);
    }

	return CSP_ERR_NONE;

}


int csp_usart_open(const csp_usart_conf_t *conf, csp_usart_callback_t rx_callback, void * user_data, sciBASE_t * return_fd) {

    sciInit();
    sciBASE_t fd;
    int res = openPort(conf->device, &fd);
    if (res != CSP_ERR_NONE) {
        return res;
    }

    sciSetBaudrate(sciREG3, conf->baudrate);
//    res = configurePort(fd, conf);
//    if (res != CSP_ERR_NONE) {
//        CloseHandle(fd);
//        return res;
//    }

//    res = setPortTimeouts(fd);
//    if (res) {
//        CloseHandle(fd);
//        return res;
//    }

    usart_context_t * ctx = csp_calloc(1, sizeof(*ctx));
//    if (ctx == NULL) {
//        csp_log_error("%s: Error allocating context, device: [%s], errno: %s", __FUNCTION__, conf->device, strerror(errno));
//        CloseHandle(fd);
//        return CSP_ERR_NOMEM;
//    }
    ctx->rx_callback = rx_callback;
    ctx->user_data = user_data;
    ctx->fd = sciREG3;
    ctx->isListening = 1;

    xTaskCreate(usart_rx_thread, "usart_rx", 50, (void *) ctx, 0, &ctx->rx_thread);

//    if (res) {
//        CloseHandle(ctx->fd);
//        csp_free(ctx);
//        return res;
//    }

    return CSP_ERR_NONE;
}
