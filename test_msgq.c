#include <stdio.h>

#include "sys/time.h"
#include "unistd.h"
#include "pthread.h"
#include "msgq.h"

///////////////////////////////////////////
#define err_msg(str, args...)           fprintf(stderr, "%s[%3d]=> " str, __FUNCTION__, __LINE__, ## args)

static struct timeval   g_start;
///////////////////////////////////////////
static void*
_thread_post(void *argv)
{
    msgq_handle_t       *pHMsgq = (msgq_handle_t*)argv;

    struct timeval  timeout;

    while( (timeout.tv_sec - g_start.tv_sec) < 10 )
    {
        int     *pValue = 0xab;
        int     ret = 0;

        ret = msgq_post_msg(pHMsgq, pValue);
        if( ret )
        {
            err_msg("return err (%d) !\n", ret);
            continue;
        }
        err_msg("cur msg cnt = %d\n", pHMsgq->cur_msg_cnt);

        gettimeofday(&timeout, NULL);
        usleep(100000);
    }
    err_msg("leave\n");
    pthread_exit(NULL);
    return 0;
}

static void*
_thread_fetch(void *argv)
{
    msgq_handle_t       *pHMsgq = (msgq_handle_t*)argv;

    struct timeval  timeout;

    while( (timeout.tv_sec - g_start.tv_sec) < 10 )
    {
        int     *pValue = 0;
        int     ret = 0;

        ret = msgq_fetch_msg(pHMsgq, (void**)&pValue);
        if( !ret )
        {
            if( pValue != 0xab )
            {
                err_msg("fetch wrong data !\n");
            }
            err_msg("cur msg cnt = %d\n", pHMsgq->cur_msg_cnt);
        }

        gettimeofday(&timeout, NULL);
        usleep(100000);
    }

    err_msg("leave\n");
    pthread_exit(NULL);
    return 0;
}
///////////////////////////////////////////
int main()
{
    pthread_t           t1, t2;
    msgq_handle_t       *pHMsgq = 0;
    msgq_init_info_t    init_info = {0};

    init_info.max_msg_num = 5;
    msgq_create_handle(&pHMsgq, &init_info);

    gettimeofday(&g_start, NULL);

    pthread_create(&t1, NULL, _thread_post, (void*)pHMsgq);
    pthread_create(&t2, NULL, _thread_fetch, (void*)pHMsgq);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("\n=> cur msg cnt = %d\n", pHMsgq->cur_msg_cnt);
    while( 0) //pHMsgq->cur_msg_cnt )
    {
        int     *pValue = 0;
        msgq_fetch_msg(pHMsgq, (void**)&pValue);
    }

    msgq_destroy_handle(&pHMsgq);
    return 0;
}
