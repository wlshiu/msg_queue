/**
 * Copyright (c) 2016 Wei-Lun Hsu. All Rights Reserved.
 */
/** @file msgq.h
 *
 * @author Wei-Lun Hsu
 * @version 0.1
 * @date 2016/01/21
 * @license
 * @description
 */

#ifndef __msgq_H_wlGESSKg_lSAH_HjND_sg4w_uQ8oh3aPvnML__
#define __msgq_H_wlGESSKg_lSAH_HjND_sg4w_uQ8oh3aPvnML__

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
//=============================================================================
//                Constant Definition
//=============================================================================

//=============================================================================
//                Macro Definition
//=============================================================================

//=============================================================================
//                Structure Definition
//=============================================================================
struct msgq_opt_desc;

/**
 *  message queue initial info
 */
typedef struct msgq_init_info
{
    int     max_msg_num;
    void    *pTunnal_info;

    int (*reset_opt_desc)(struct msgq_opt_desc *pDesc, void *pTunnel_info);
} msgq_init_info_t;

/**
 *  message queue handle
 */
typedef struct msgq_handle
{
    int     max_msg_cnt;
    int     cur_msg_cnt;
} msgq_handle_t;

/**
 *  message queue operator description
 */
typedef struct msgq_opt_desc
{
    char    name[4];
    void    *pExtra_data[2];

    int     (*post_msg)(msgq_handle_t *pHMsgq, void *pMsg_data, struct msgq_opt_desc *pDesc);
    int     (*fetch_msg)(msgq_handle_t *pHMsgq, void **ppMsg_data, struct msgq_opt_desc *pDesc);
    void    (*clean_msg)(msgq_handle_t *pHMsgq, struct msgq_opt_desc *pDesc);

    int     (*emergency_proc)(msgq_handle_t *pHMsgq, void *pMsg_data, struct msgq_opt_desc *pDesc);
} msgq_opt_desc_t;
//=============================================================================
//                Global Data Definition
//=============================================================================

//=============================================================================
//                Private Function Definition
//=============================================================================

//=============================================================================
//                Public Function Definition
//=============================================================================
int
msgq_create_handle(
    msgq_handle_t       **ppHMsgq,
    msgq_init_info_t    *pInit_info);


void
msgq_destroy_handle(
    msgq_handle_t   **ppHMsgq);


int
msgq_post_msg(
    msgq_handle_t   *pHMsgq,
    void            *pMsg_data);


int
msgq_fetch_msg(
    msgq_handle_t   *pHMsgq,
    void            **ppMsg_data);


int
msgq_emergency_proc(
    msgq_handle_t   *pHMsgq,
    void            *pMsg_data);


#ifdef __cplusplus
}
#endif

#endif
