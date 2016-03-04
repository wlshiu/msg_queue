/**
 * Copyright (c) 2016 Wei-Lun Hsu. All Rights Reserved.
 */
/** @file prior_msgq.h
 *
 * @author Wei-Lun Hsu
 * @version 0.1
 * @date 2016/03/03
 * @license
 * @description
 */

#ifndef __prior_msgq_H_w9bJV6Pw_lnG2_HlJj_souE_uFQSjfMZnC5G__
#define __prior_msgq_H_w9bJV6Pw_lnG2_HlJj_souE_uFQSjfMZnC5G__

#ifdef __cplusplus
extern "C" {
#endif


//=============================================================================
//                Constant Definition
//=============================================================================

//=============================================================================
//                Macro Definition
//=============================================================================

//=============================================================================
//                Structure Definition
//=============================================================================
/**
 *  message queue initial info
 */
typedef struct msgq_init_info
{
    int     max_msg_num;
    void    *pTunnal_info;

} msgq_init_info_t;

/**
 *  message queue handle
 */
typedef struct msgq_handle
{
    int     max_msg_cnt;
    int     cur_msg_cnt;

} msgq_handle_t;
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


int
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
msgq_sort_msg(
    msgq_handle_t   *pHMsgq,
    int (*fp_compare)(void *a, void *b));


#ifdef __cplusplus
}
#endif

#endif