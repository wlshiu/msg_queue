#include <stdio.h>
#include <stdlib.h>

#include "err_code_def.h"
#include "prior_msgq.h"

#include <unistd.h>
#include "pthread.h"



/**
 * // add to prior_msgq.c
 * extern pthread_mutex_t  g_log_mutex;
 * #undef err_msg
 * #define err_msg(string, args...)    do{ pthread_mutex_lock(&g_log_mutex);                                       \
 *                                         fprintf(stderr, "[%s,#%4d]  " string , __FUNCTION__, __LINE__, ## args);  \
 *                                         pthread_mutex_unlock(&g_log_mutex);                                     \
 *                                     }while(0)
*/

pthread_mutex_t  g_log_mutex = PTHREAD_MUTEX_INITIALIZER;

static int
_compare(void *a, void *b)
{
    return ((int)a - (int)b);
}

void*
thread_post(void *arg)
{
    msgq_handle_t       *pHMsgq = (msgq_handle_t*)arg;
    int                 cnt = 0, rval = 0;

    for(cnt = 0; cnt < 4; cnt++)
    {
        rval = msgq_post_msg(pHMsgq, (void*)(4 - cnt));
        if( !rval )
        {
            pthread_mutex_lock(&g_log_mutex);
            printf(" post:  %d\n", 4-cnt);
            pthread_mutex_unlock(&g_log_mutex);
        }
    }

    msgq_sort_msg(pHMsgq, _compare);

    while(cnt < 10 )
    {
        rval = msgq_post_msg(pHMsgq, (void*)cnt);
        if( !rval )
        {
            pthread_mutex_lock(&g_log_mutex);
            printf(" post:  %d\n", cnt);
            pthread_mutex_unlock(&g_log_mutex);
        }
        cnt++;
    }

    pthread_exit(NULL);
    return 0;
}


void*
thread_fetch(void *arg)
{
    msgq_handle_t       *pHMsgq = (msgq_handle_t*)arg;
    int                 cnt = 0;

    while(cnt < 10 )
    {
        int     tmp = 0;
        msgq_fetch_msg(pHMsgq, (void*)&tmp);

        pthread_mutex_lock(&g_log_mutex);
        if( tmp )
            printf(" fetch:  %d\n", tmp);
        pthread_mutex_unlock(&g_log_mutex);
        cnt++;
        usleep(1000);
    }

    pthread_exit(NULL);
    return 0;
}

int main()
{
    msgq_handle_t       *pHMsgq = 0;
    msgq_init_info_t    init_info = {0};
    pthread_t           tid_1, tid_2;

    init_info.max_msg_num = 5;
    msgq_create_handle(&pHMsgq, &init_info);

    pthread_mutex_lock(&g_log_mutex);
    printf(" pHMsgq:  %p\n", pHMsgq);
    pthread_mutex_unlock(&g_log_mutex);

    pthread_create(&tid_1, NULL, thread_post, pHMsgq);
    pthread_create(&tid_2, NULL, thread_fetch, pHMsgq);

    pthread_join(tid_1, NULL);
    pthread_join(tid_2, NULL);

    msgq_destroy_handle(&pHMsgq);

    return 0;
}
