/**
 * Copyright (c) 2016 Wei-Lun Hsu. All Rights Reserved.
 */
/** @file msgq.c
 *
 * @author Wei-Lun Hsu
 * @version 0.1
 * @date 2016/01/21
 * @license
 * @description
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "msgq.h"
#include "pthread.h"

//=============================================================================
//                Constant Definition
//=============================================================================

//=============================================================================
//                Macro Definition
//=============================================================================
#define _verify_handle(handle, err_code)        \
            do{ if(handle==NULL){               \
                err("Null pointer !!\n");   \
                return err_code;}               \
            }while(0)

#ifndef MEMBER_OFFSET
    #define MEMBER_OFFSET(type, member)     (uintptr_t)&(((type *)0)->member)
#endif

#ifndef STRUCTURE_POINTER
    #define STRUCTURE_POINTER(type, ptr, member)    (type*)((uintptr_t)ptr - MEMBER_OFFSET(type, member))
#endif


#if 1
    #define err(str, args...)           fprintf(stderr, "%s[%3d]=> " str, __FUNCTION__, __LINE__, ## args)
#else
    #define err(str, args...)
#endif
//=============================================================================
//                Structure Definition
//=============================================================================
/**
 *  message node
 */
typedef struct msg_node
{
    struct msg_node    *next;

    void                *pData;
    void                *pBase_addr;

} msg_node_t;

/**
 *
 */
typedef struct msgq_dev
{
    msgq_handle_t           hMsgq;

    pthread_mutex_t         mutex;
    void                    *pTunnal_info;

    msgq_opt_desc_t         msgq_opt_desc;

    int                    max_node_num;
    int                    cur_node_cnt;
    msg_node_t             *pMsg_node_head;
    msg_node_t             *pMsg_node_cur;

} msgq_dev_t;
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
    msgq_init_info_t    *pInit_info)
{
    int         result = -1;
    msgq_dev_t  *pDev = 0;

    do{
        if( !ppHMsgq || !pInit_info )
        {
            err("wring parameters !\n");
            break;
        }

        if( !(pDev = malloc(sizeof(msgq_dev_t))) )
        {
            err("malloc fail !!\n");
            break;
        }
        memset(pDev, 0x0, sizeof(msgq_dev_t));

        //-----------------------
        // init parameters
        pthread_mutex_init(&pDev->mutex, NULL);

        pDev->max_node_num = pDev->hMsgq.max_msg_cnt = pInit_info->max_msg_num;
        pDev->pTunnal_info = pInit_info->pTunnal_info;

        if( pInit_info->reset_opt_desc )
            pInit_info->reset_opt_desc(&pDev->msgq_opt_desc, pDev->pTunnal_info);

        *ppHMsgq = &pDev->hMsgq;

        result = 0;
    }while(0);

    return result;
}

void
msgq_destroy_handle(
    msgq_handle_t   **ppHMsgq)
{
    msgq_dev_t  *pDev = 0;
    if( !ppHMsgq || !(*ppHMsgq) )
        return;

    pDev = STRUCTURE_POINTER(msgq_dev_t, (*ppHMsgq), hMsgq);
    do{
        pthread_mutex_lock(&pDev->mutex);
        if( pDev->msgq_opt_desc.clean_msg )
        {
            pDev->msgq_opt_desc.clean_msg(*ppHMsgq, &pDev->msgq_opt_desc);
        }
        else
        {
            if( pDev->cur_node_cnt )
            {
                msg_node_t     *pCur_node = pDev->pMsg_node_head;

                err("!! Remain %d msg nodes, it may lead to leak !\n", pDev->cur_node_cnt);
                while( pCur_node )
                {
                    msg_node_t     *pTmp_node = pCur_node;
                    pCur_node = pCur_node->next;
                    free(pTmp_node);
                }
            }
        }

        *ppHMsgq = 0;

        pthread_mutex_unlock(&pDev->mutex);
        pthread_mutex_destroy(&pDev->mutex);
        free(pDev);
    }while(0);

    return;
}

int
msgq_post_msg(
    msgq_handle_t   *pHMsgq,
    void            *pMsg_data)
{
    int         result = -1;
    msgq_dev_t  *pDev = 0;

    _verify_handle(pHMsgq, -101);
    _verify_handle(pMsg_data, -102);

    pDev = STRUCTURE_POINTER(msgq_dev_t, pHMsgq, hMsgq);
    pthread_mutex_lock(&pDev->mutex);
    do{
        msg_node_t     *pNew_node = 0;

        if( pDev->cur_node_cnt == pDev->max_node_num )
        {
            err("msg full (num: %d) !!!\n", pDev->max_node_num);
            break;
        }

        // user need to handle post and fatch by self
        if( pDev->msgq_opt_desc.post_msg &&
            pDev->msgq_opt_desc.fetch_msg )
        {
            result = pDev->msgq_opt_desc.post_msg(pHMsgq, pMsg_data, &pDev->msgq_opt_desc);
            pDev->cur_node_cnt = pHMsgq->cur_msg_cnt;
            break;
        }

        // default post flow
        pNew_node = malloc(sizeof(msg_node_t));
        if( !pNew_node )
        {
            err("malloc node fail !\n");
            break;
        }
        memset(pNew_node, 0x0, sizeof(msg_node_t));
        pNew_node->pData = pMsg_data;

        if( !pDev->pMsg_node_head )
        {
            pDev->pMsg_node_head = pDev->pMsg_node_cur = pNew_node;
            pDev->cur_node_cnt = 1;
            pDev->hMsgq.cur_msg_cnt = pDev->cur_node_cnt;
        }
        else
        {
            pDev->pMsg_node_cur->next = pNew_node;
            pDev->pMsg_node_cur = pDev->pMsg_node_cur->next;
            pDev->cur_node_cnt++;
            pDev->hMsgq.cur_msg_cnt = pDev->cur_node_cnt;
        }

        result = 0;
    }while(0);

    pthread_mutex_unlock(&pDev->mutex);

    return result;
}

int
msgq_fetch_msg(
    msgq_handle_t   *pHMsgq,
    void            **ppMsg_data)
{
    int         result = -1;
    msgq_dev_t  *pDev = 0;

    _verify_handle(pHMsgq, -101);
    _verify_handle(ppMsg_data, -102);

    pDev = STRUCTURE_POINTER(msgq_dev_t, pHMsgq, hMsgq);
    pthread_mutex_lock(&pDev->mutex);
    do{
        msg_node_t     *pCur_node = 0;

        // no msg
        if( !pDev->cur_node_cnt )
        {
            result = -103;
            break;
        }

        // user need to handle post and fatch by self
        if( pDev->msgq_opt_desc.post_msg &&
            pDev->msgq_opt_desc.fetch_msg )
        {
            result = pDev->msgq_opt_desc.fetch_msg(pHMsgq, ppMsg_data, &pDev->msgq_opt_desc);
            pDev->cur_node_cnt = pHMsgq->cur_msg_cnt;
            break;
        }

        // default fatch flow
        pCur_node = pDev->pMsg_node_head;
        if( !pCur_node )
        {
            *ppMsg_data = 0;
            pDev->cur_node_cnt = 0;
            pDev->hMsgq.cur_msg_cnt = pDev->cur_node_cnt;
            break;
        }

        pDev->pMsg_node_head = pCur_node->next;
        pDev->cur_node_cnt--;
        pDev->cur_node_cnt = (pDev->cur_node_cnt < 0)
                           ? 0 : pDev->cur_node_cnt;

        pDev->hMsgq.cur_msg_cnt = pDev->cur_node_cnt;

        // pData should br freed by user.
        *ppMsg_data = pCur_node->pData;

        free(pCur_node);

        result = 0;
    }while(0);

    pthread_mutex_unlock(&pDev->mutex);

    return result;
}

int
msgq_emergency_proc(
    msgq_handle_t   *pHMsgq,
    void            *pMsg_data)
{
    int         result = -1;
    msgq_dev_t  *pDev = 0;

    _verify_handle(pHMsgq, -101);
    _verify_handle(pMsg_data, -102);

    pDev = STRUCTURE_POINTER(msgq_dev_t, pHMsgq, hMsgq);
    pthread_mutex_lock(&pDev->mutex);

    if( pDev->msgq_opt_desc.emergency_proc )
    {
        result = pDev->msgq_opt_desc.emergency_proc(pHMsgq, pMsg_data, &pDev->msgq_opt_desc);
    }

    pthread_mutex_unlock(&pDev->mutex);

    return result;
}
