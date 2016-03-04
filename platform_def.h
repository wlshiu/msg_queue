/**
 * Copyright (c) 2016 Wei-Lun Hsu. All Rights Reserved.
 */
/** @file platform_def.h
 *
 * @author Wei-Lun Hsu
 * @version 0.1
 * @date 2016/03/01
 * @license
 * @description
 */

#ifndef __platform_def_H_wxyM9o78_lOYO_HRUv_szQl_uuWSCzz8P9fT__
#define __platform_def_H_wxyM9o78_lOYO_HRUv_szQl_uuWSCzz8P9fT__

#ifdef __cplusplus
extern "C" {
#endif


#include <stdio.h>
#include <string.h>
//=============================================================================
//                Constant Definition
//=============================================================================
#define ENABLE_PTHREAD          1

/**
 * 64/32 bits OS definition
 */
#if defined(OS64BITS)
    typedef unsigned __int64 uint_ptr_t;
#else
    typedef unsigned int uint_ptr_t;
#endif


/**
 * pthread fucntion
 */
#if ENABLE_PTHREAD
    #include "pthread.h"
    #define _thread_create(pThread, pAttr, fnRounting, pArgv)       pthread_create(pThread, pAttr, fnRounting, pArgv)
    #define _thread_exit(ptr)                                       pthread_exit(ptr)
    #define _thread_join(pThread, attr)                             do{ if(pThread)pthread_join((*pThread), attr);}while(0)
    #define _mutex_init(mutex)                                      do{ if(!mutex){\
                                                                        pthread_mutex_init(&mutex, NULL);\
                                                                        /*printf(" mutex_init: %s, 0x%x\n", #mutex, mutex);*/}\
                                                                    }while(0)
    #define _mutex_deinit(mutex)                                    do{if(mutex){pthread_mutex_destroy(&mutex);mutex=0;}}while(0)
    #define _mutex_lock(mutex)                                      do{ if(mutex){\
                                                                        pthread_mutex_lock(&mutex);\
                                                                        /*dbg_msg("  lock %s(mutex=0x%x)\n", __FUNCTION__, mutex);*/}\
                                                                    }while(0)
    #define _mutex_unlock(mutex)                                    do{ if(mutex){\
                                                                        /*dbg_msg("  unlock %s(mutex=0x%x)\n", __FUNCTION__, mutex);*/\
                                                                        pthread_mutex_unlock(&mutex);}\
                                                                    }while(0)
#else
    #define pthread_t                                               unsigned int
    #define pthread_mutex_t                                         unsigned int
    #define _thread_create(pThread, pAttr, fnRounting, pArgv)       do{ *pThread=-1;}while(0)
    #define _thread_exit(ptr)
    #define _mutex_init(mutex)
    #define _mutex_init(mutex)
    #define _mutex_deinit(mutex)
    #define _mutex_lock(mutex)
    #define _mutex_unlock(mutex)
#endif
//=============================================================================
//                Macro Definition
//=============================================================================
/**
 *  message log
 */
#if 1
    #define trace()                     fprintf(stderr, "%s[#%d] \n", __FUNCTION__, __LINE__)

    #define err_msg(string, args...)    do{ fprintf(stderr, "%s [#%d] => " string, __FUNCTION__, __LINE__, ## args);}while(0)

    #define dbg_msg(string, args...)    do{ fprintf(stderr, "%s [#%d] => " string, __FUNCTION__, __LINE__, ## args);}while(0)

#else
    #define err_msg(string, args...)    do{ fprintf(stderr, "%s [#%d] => " string, __FUNCTION__, __LINE__, ## args);}while(0)
    #define dbg_msg(string, args...)
#endif

#ifndef MEMBER_OFFSET
    #define MEMBER_OFFSET(type, member)     (uint_ptr_t)&(((type *)0)->member)
#endif

#ifndef STRUCTURE_POINTER
    #define STRUCTURE_POINTER(type, ptr, member)    (type*)((uint_ptr_t)ptr - MEMBER_OFFSET(type, member))
#endif


/**
 * handle check function
 */
#define _verify_handle(pHandle, err_code)   do{if(!pHandle){\
                                                err_msg("%s Null pointer !!\n", #pHandle);\
                                                return err_code;}\
                                            }while(0)

/**
 *  register macro
 */
#define DEFINE_REGISTER_TEMPLATE(descType, id_enum_type)     \
    static descType *first_##descType##_item = 0;       \
    static void                                         \
    _register_##descType##_item(                        \
        descType     *format){                          \
        descType **p;                                   \
        p = &first_##descType##_item;                   \
        while (*p != 0) p = &(*p)->next;                \
        *p = format;                                    \
        format->next = 0;                               \
    }                                                   \
    static descType*                                    \
    descType##_next(                                    \
        descType  *f){                                  \
        if(f) return f->next;                           \
        else  return first_##descType##_item;           \
    }                                                   \
    static descType*                                    \
    descType##_find(                                    \
        id_enum_type   id){                             \
        descType *dev = 0;                              \
        while((dev = descType##_next(dev)))             \
        if( dev->id == (unsigned int)id ) return dev;   \
        return 0;                                       \
    }

#if 0
    #define REGISTER_ITEM(type, X, x) { \
        extern type type##_##x##_desc; \
        if(CONFIG_##type##_##X##_DESC)  _register_##type##_item(&type##_##x##_desc); }
#else
    #define REGISTER_ITEM(type, x) { \
        extern type type##_##x##_desc; \
        _register_##type##_item(&type##_##x##_desc); }

#endif

#define FIND_DESC_ITEM(descType, id)    descType##_find(id)
//=============================================================================
//                Structure Definition
//=============================================================================

//=============================================================================
//                Global Data Definition
//=============================================================================

//=============================================================================
//                Private Function Definition
//=============================================================================

//=============================================================================
//                Public Function Definition
//=============================================================================

#ifdef __cplusplus
}
#endif

#endif
