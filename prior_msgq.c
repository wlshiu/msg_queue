/**
 * Copyright (c) 2016 Wei-Lun Hsu. All Rights Reserved.
 */
/** @file prior_msgq.c
 *
 * @author Wei-Lun Hsu
 * @version 0.1
 * @date 2016/03/03
 * @license
 * @description
 */

#include <stdlib.h>

#include "err_code_def.h"
#include "platform_def.h"
#include "prior_msgq.h"

//=============================================================================
//                Constant Definition
//=============================================================================

//=============================================================================
//                Macro Definition
//=============================================================================
extern pthread_mutex_t  g_log_mutex;
#undef err_msg
#define err_msg(string, args...)    do{ pthread_mutex_lock(&g_log_mutex);                                       \
                                        fprintf(stderr, "[%s,#%4d]  " string , __FUNCTION__, __LINE__, ## args);  \
                                        pthread_mutex_unlock(&g_log_mutex);                                     \
                                    }while(0)
//=============================================================================
//                Structure Definition
//=============================================================================
/**
 *  message node
 */
struct msg_node;
typedef struct msg_node
{
    void    *pTunnel_info;
    void    *pData;

} msg_node_t;


/**
 *  message device
 */
typedef struct msgq_dev
{
    msgq_handle_t       hMsgq;

    pthread_mutex_t     mutex;

    int                 node_idx_w;
    int                 node_idx_r;
    int                 node_max_num;
    int                 node_cnt;
    msg_node_t          **ppNode_list;

    // sort func
    int (*fp_compare)(void *a, void *b);

} msgq_dev_t;
//=============================================================================
//                Global Data Definition
//=============================================================================

//=============================================================================
//                Private Function Definition
//=============================================================================
/*
 *  return -1 => a < b
 *  return 0  => a = b
 *  return 1  => a > b
 */
static int
_def_compare(
    const void  *a,
    const void  *b)
{
    msg_node_t     *pNode_a = *((msg_node_t**)a);
    msg_node_t     *pNode_b = *((msg_node_t**)b);
    msgq_dev_t     *pDev = 0;

    // if( !pNode_a )
    //     return ;
    // else if( !pNode_b )
    //     return ;

    pDev = (msgq_dev_t*)pNode_a->pTunnel_info;

    return pDev->fp_compare(pNode_a->pData, pNode_b->pData);
}
//=============================================================================
//                Public Function Definition
//=============================================================================
int
msgq_create_handle(
    msgq_handle_t       **ppHMsgq,
    msgq_init_info_t    *pInit_info)
{
    int             result = ERRCODE_OK;
    msgq_dev_t      *pDev = 0;

    do{
        if( !ppHMsgq || !pInit_info )
        {
            result = ERRCODE_NULL_POINTER;
            err_msg("err, NULL pointer !!\n");
            break;
        }

        if( *ppHMsgq != 0 )
        {
            result = ERRCODE_INVALID_PARAM;
            err_msg("err, Exist msgq handle !!\n");
            break;
        }

        if( !pInit_info->max_msg_num )
        {
            result = ERRCODE_INVALID_PARAM;
            err_msg("err, invalid parameter !!\n");
            break;
        }

        // -------------------------------
        // create device
        if( !(pDev = malloc(sizeof(msgq_dev_t))) )
        {
            result = ERRCODE_MALLOC_FAIL;
            err_msg("err, malloc fail !\n");
            break;
        }
        memset(pDev, 0x0, sizeof(msgq_dev_t));

        // create node list
        if( !(pDev->ppNode_list = malloc(pInit_info->max_msg_num*sizeof(msg_node_t*))) )
        {
            result = ERRCODE_MALLOC_FAIL;
            err_msg("err, malloc fail !\n");
            break;
        }
        memset(pDev->ppNode_list, 0x0, pInit_info->max_msg_num*sizeof(msg_node_t*));

        //---------------------------------
        // init parameter
        pDev->hMsgq.max_msg_cnt = pInit_info->max_msg_num;
        pDev->node_max_num      = pInit_info->max_msg_num;
        pDev->node_idx_r        = 0;//-1;

        _mutex_init(pDev->mutex);

        //----------------------
        (*ppHMsgq) = &pDev->hMsgq;

    }while(0);

    if( result )
    {
        msgq_handle_t   *pHMsgq = &pDev->hMsgq;
        msgq_destroy_handle(&pHMsgq);

        err_msg("err %d !\n", result);
    }

    return result;
}

int
msgq_destroy_handle(
    msgq_handle_t   **ppHMsgq)
{
    int      result = ERRCODE_OK;

    if( ppHMsgq && *ppHMsgq )
    {
        int                 i;
        msgq_dev_t          *pDev = STRUCTURE_POINTER(msgq_dev_t, (*ppHMsgq), hMsgq);
        pthread_mutex_t     mutex;

        _mutex_lock(pDev->mutex);
        // ToDo: How to handle un-fetched nodes in list
        for(i = 0; i < pDev->node_max_num; i++)
        {
            if( pDev->ppNode_list[i] )
                free(pDev->ppNode_list[i]);
        }

        if( pDev->ppNode_list )
            free(pDev->ppNode_list);

        mutex = pDev->mutex;

        free(pDev);

        _mutex_unlock(mutex);
        _mutex_deinit(mutex);

        *ppHMsgq = 0;
    }

    return result;
}

int
msgq_post_msg(
    msgq_handle_t   *pHMsgq,
    void            *pMsg_data)
{
    int             result = ERRCODE_OK;
    msgq_dev_t      *pDev = 0;

    _verify_handle(pHMsgq, ERRCODE_NULL_POINTER);
    _verify_handle(pMsg_data, ERRCODE_NULL_POINTER);

    pDev = STRUCTURE_POINTER(msgq_dev_t, pHMsgq, hMsgq);

    _mutex_lock(pDev->mutex);
    do{
        msg_node_t     *pNew_node = 0;

        if( pDev->node_cnt == pDev->hMsgq.max_msg_cnt )
        {
            err_msg("err, queue full !!\n");
            result = ERRCODE_NO_SLOT;
            break;
        }

        // create node and insert to list
        pNew_node = malloc(sizeof(msg_node_t));
        if( !pNew_node )
        {
            result = ERRCODE_MALLOC_FAIL;
            err_msg("err, malloc fail !\n");
            break;
        }
        memset(pNew_node, 0x0, sizeof(msg_node_t));
        pNew_node->pTunnel_info = (void*)pDev;
        pNew_node->pData        = pMsg_data;

        if( pDev->ppNode_list[pDev->node_idx_w] )
            err_msg("!! something wrong, overflow\n");

        err_msg("  node_idx_w = %d\n", pDev->node_idx_w);
        pDev->ppNode_list[pDev->node_idx_w] = pNew_node;

        if( (++pDev->node_idx_w) == pDev->node_max_num )
            pDev->node_idx_w = 0;

        pDev->node_cnt++;
        pDev->node_cnt = (pDev->node_cnt > pDev->node_max_num)
                       ? pDev->node_max_num : pDev->node_cnt;
        pDev->hMsgq.cur_msg_cnt = pDev->node_cnt;
    }while(0);

    _mutex_unlock(pDev->mutex);

    if( result )
    {
        err_msg("err %d !\n", result);
    }
    return result;
}


int
msgq_fetch_msg(
    msgq_handle_t   *pHMsgq,
    void            **ppMsg_data)
{
    int             result = ERRCODE_OK;
    msgq_dev_t      *pDev = 0;

    _verify_handle(pHMsgq, ERRCODE_NULL_POINTER);
    _verify_handle(ppMsg_data, ERRCODE_NULL_POINTER);

    pDev = STRUCTURE_POINTER(msgq_dev_t, pHMsgq, hMsgq);

    _mutex_lock(pDev->mutex);
    do{
        msg_node_t     *pCur_node = 0;

        if( pDev->node_cnt == 0 )
        {
            *ppMsg_data = NULL;
            err_msg("No node in queue !\n");
            break;
        }

        // pop from list
        err_msg("  node_idx_r = %d\n", pDev->node_idx_r);
        pCur_node = pDev->ppNode_list[pDev->node_idx_r];
        pDev->ppNode_list[pDev->node_idx_r] = 0;
        if( !pCur_node )
            err_msg("something wrong, fetch NULL !!\n");

        *ppMsg_data = pCur_node->pData;
        free(pCur_node);

        if( (++pDev->node_idx_r) == pDev->node_max_num )
            pDev->node_idx_r = 0;

        pDev->node_cnt--;
        pDev->node_cnt = (pDev->node_cnt < 0)
                       ? 0 : pDev->node_cnt;
        pDev->hMsgq.cur_msg_cnt = pDev->node_cnt;
    }while(0);

    _mutex_unlock(pDev->mutex);

    if( result )
    {
        err_msg("err %d !\n", result);
    }
    return result;
}


int
msgq_sort_msg(
    msgq_handle_t   *pHMsgq,
    int (*fp_compare)(void *a, void *b))
{
    int             result = ERRCODE_OK;
    msgq_dev_t      *pDev = 0;

    _verify_handle(pHMsgq, ERRCODE_NULL_POINTER);
    _verify_handle(fp_compare, ERRCODE_NULL_POINTER);

    pDev = STRUCTURE_POINTER(msgq_dev_t, pHMsgq, hMsgq);

    _mutex_lock(pDev->mutex);
    do{
        int             i, j;
        msg_node_t      **ppTmp_list = 0;

        if( pDev->node_cnt == 0 )
        {
            err_msg("No node in queue !\n");
            break;
        }

        pDev->fp_compare = fp_compare;

        if( !(ppTmp_list = malloc(pDev->node_cnt*sizeof(msg_node_t*))) )
        {
            result = ERRCODE_MALLOC_FAIL;
            err_msg("err, malloc fail !\n");
            break;
        }
        memset(ppTmp_list, 0x0, pDev->node_cnt*sizeof(msg_node_t*));

        j = 0;
        for(i = 0; i < pDev->node_max_num; i++)
        {
            if( !pDev->ppNode_list[i] )
                continue;

            ppTmp_list[j++] = pDev->ppNode_list[i];
        }

        if( j > pDev->node_cnt )
            err_msg(" !!! overflow \n");

        qsort(ppTmp_list, pDev->node_cnt, sizeof(msg_node_t*), _def_compare);

        // refresh info
        memset(pDev->ppNode_list, 0x0, pDev->node_max_num*sizeof(msg_node_t*));
        memcpy(pDev->ppNode_list, ppTmp_list, pDev->node_cnt*sizeof(msg_node_t*));
        free(ppTmp_list);

        pDev->node_idx_r = 0;
        pDev->node_idx_w = pDev->node_cnt;

        for(i = 0; i < pDev->node_max_num; i++)
        {
            if( !pDev->ppNode_list[i] )
                continue;

            err_msg(" after sort: %d-th, %d\n", i, (int)pDev->ppNode_list[i]->pData);
        }

    }while(0);

    _mutex_unlock(pDev->mutex);

    if( result )
    {
        err_msg("err %d !\n", result);
    }
    return result;
}

#if 0
// msg_data will make memory leak
int
msgq_erase_all_msg(
    msgq_handle_t   *pHMsgq)
{
    int             result = ERRCODE_OK;
    msgq_dev_t      *pDev = 0;

    _verify_handle(pHMsgq, ERRCODE_NULL_POINTER);

    pDev = STRUCTURE_POINTER(msgq_dev_t, pHMsgq, hMsgq);

    _mutex_lock(pDev->mutex);
    do{
        int     i;
        for(i = 0; i < pDev->node_max_num; i++)
            pDev->ppNode_list[i] = 0;

        pDev->node_idx_r = 0;
        pDev->node_idx_w = 0;
        pDev->node_cnt   = 0;
    }while(0);

    _mutex_unlock(pDev->mutex);

    if( result )
    {
        err_msg("err %d !\n", result);
    }
    return result;
}
#endif
