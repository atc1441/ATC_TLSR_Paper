#ifndef __MIBLE_LOG_H__
#define __MIBLE_LOG_H__
// Copyright [2017] [Beijing Xiaomi Mobile Software Co., Ltd]
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

//#include <stdbool.h>
#include <stdint.h>

#include "mible_port.h"

#ifdef MI_LOG_ENABLED
/**
 * Log error   level    :1
 * Log warning level    :2
 * Log info    level    :3
 * Log debug   level    :4
 */

#define MI_LOG_LEVEL              4
#define MI_LOG_COLORS_ENABLE      1

#include "mible_log_internal.h"

#define MI_LOG_ERROR(...)                     	MI_LOG_INTERNAL_ERROR(__VA_ARGS__)
#define MI_LOG_WARNING(...)                   	MI_LOG_INTERNAL_WARNING( __VA_ARGS__)
#define MI_LOG_INFO(...)                      	MI_LOG_INTERNAL_INFO( __VA_ARGS__)
#define MI_LOG_DEBUG(...) 		                MI_LOG_INTERNAL_DEBUG( __VA_ARGS__)

#else // MI_LOG_ENABLED
#define MI_LOG_ERROR(...)
#define MI_LOG_WARNING(...)
#define MI_LOG_INFO(...)
#define MI_LOG_DEBUG(...)
#endif // MI_LOG_ENABLED


#ifdef MI_ASSERT

#define MI_ERR_HANDLER(ERR_CODE)                                       \
    do                                                                 \
    {                                                                  \
        MI_LOG_ERROR("Error code 0x%04X.  %s:%d\n", ERR_CODE, (uint32_t)__FILE__, __LINE__);  \
    } while (0)

#define MI_ERR_CHECK(ERR_CODE)                              \
    do                                                      \
    {                                                       \
        const uint32_t LOCAL_ERR_CODE = (ERR_CODE);         \
        if (LOCAL_ERR_CODE != 0)                            \
        {                                                   \
            MI_ERR_HANDLER(LOCAL_ERR_CODE);                 \
        }                                                   \
    } while (0)

#define MI_ERR_TEST(ERR_CODE, EXPECT)                       \
    do                                                      \
    {                                                       \
        const uint32_t LOCAL_ERR_CODE = (ERR_CODE);         \
        if (LOCAL_ERR_CODE != (EXPECT))                     \
        {                                                   \
            MI_ERR_HANDLER(LOCAL_ERR_CODE);                 \
        }                                                   \
    } while (0)
#else // MI_ASSERT

#define MI_ERR_CHECK(ERR_CODE)
#define MI_ERR_TEST(ERR_CODE, EXPECT)                       
#endif // MI_ASSERT



#endif // __MIBLE_LOG_H__
