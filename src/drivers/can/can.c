/*
 * can.c
 *
 *  Created on: May 24, 2020
 *      Author: Andrew
 */

#include "csp/interfaces/csp_if_can.h"
#include <csp/csp_platform.h>
#include "HL_can.h"

typedef struct {
    char name[CSP_IFLIST_NAME_MAX + 1];
    csp_iface_t iface;
    csp_can_interface_data_t ifdata;
} can_context_t;

can_context_t * ctx;

static int csp_can_tx_frame(void * driver_data, uint32_t id, const uint8_t * data, uint8_t dlc)
{
    if (dlc > 8) {
        return CSP_ERR_INVAL;
    }

//    canUpdateID(canREG1, canMESSAGE_BOX1, id); // need to experiment with this later
    canTransmit(canREG1, canMESSAGE_BOX1, data);

    return CSP_ERR_NONE;
}

void canMessageNotification(canBASE_t *node, uint32 messageBox)
{
    uint8 data;
    uint32 id;
    CSP_BASE_TYPE woken = 1;
     if(node==canREG1)
     {
         canGetData(canREG1, canMESSAGE_BOX2, (uint8 * )&data); /* copy to RAM */
         id = canGetID(node, messageBox);
         csp_can_rx(&ctx->iface, id, &data, 8, &woken);
     }
     if(node==canREG2)
     {
         canGetData(canREG2, canMESSAGE_BOX2, (uint8 * )&data); /* copy to RAM */
         id = canGetID(node, messageBox);
         csp_can_rx(&ctx->iface, id, &data, 8, &woken);
     }
}

int can_init(csp_iface_t ** return_iface) {
    _enable_IRQ_interrupt_();
    canInit();
    canEnableloopback(canREG1, Internal_Lbk);
    canEnableloopback(canREG2, Internal_Lbk);
    const char *ifaceName = "can bus";
    csp_can_interface_data_t ifdata;
    ctx = pvPortMalloc(sizeof(*ctx));
    strncpy(ctx->name, ifaceName, sizeof(ctx->name) - 1);
    ctx->iface.name = ctx->name;
    ctx->iface.interface_data = &ctx->ifdata;
    ctx->iface.driver_data = ctx;
    ctx->ifdata.tx_func = csp_can_tx_frame;
    int res = csp_can_add_interface(&ctx->iface);
    if(res != CSP_ERR_NONE) {
        free(ctx);
        return res;
    }
    if (return_iface) {
        *return_iface = &ctx->iface;
    }
    return CSP_ERR_NONE;
}

