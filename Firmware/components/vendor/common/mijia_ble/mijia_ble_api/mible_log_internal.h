#ifndef __MIBLE_LOG_INTERNAL__H__
#define __MIBLE_LOG_INTERNAL__H__

#ifndef MI_LOG_COLORS_ENABLE
#define MI_LOG_COLORS_ENABLE      0
#endif

#ifndef MI_LOG_MODULE_NAME
#define MI_LOG_MODULE_NAME        ""
#endif

#define MI_LOG_LEVEL_ERROR        1U
#define MI_LOG_LEVEL_WARNING      2U
#define MI_LOG_LEVEL_INFO         3U
#define MI_LOG_LEVEL_DEBUG        4U

#define MI_LOG_COLOR_DEFAULT      "\x1B[0m"
#define MI_LOG_COLOR_BLACK        "\x1B[1;30m"
#define MI_LOG_COLOR_RED          "\x1B[1;31m"
#define MI_LOG_COLOR_GREEN        "\x1B[1;32m"
#define MI_LOG_COLOR_YELLOW       "\x1B[1;33m"
#define MI_LOG_COLOR_BLUE         "\x1B[1;34m"
#define MI_LOG_COLOR_MAGENTA      "\x1B[1;35m"
#define MI_LOG_COLOR_CYAN         "\x1B[1;36m"
#define MI_LOG_COLOR_WHITE        "\x1B[1;37m"

#if MI_LOG_COLORS_ENABLE
#define MI_LOG_ERROR_COLOR   MI_LOG_COLOR_RED
#define MI_LOG_WARNING_COLOR MI_LOG_COLOR_YELLOW
#define MI_LOG_INFO_COLOR    MI_LOG_COLOR_DEFAULT
#define MI_LOG_DEBUG_COLOR   MI_LOG_COLOR_GREEN
#else // MI_LOG_COLORS_ENABLE
#define MI_LOG_ERROR_COLOR   
#define MI_LOG_WARNING_COLOR 
#define MI_LOG_INFO_COLOR
#define MI_LOG_DEBUG_COLOR   
#endif // MI_LOG_COLORS_ENABLE

#define MI_LOG_BREAK      ": "

#define MI_ERROR_PREFIX   MI_LOG_ERROR_COLOR "[E] " MI_LOG_MODULE_NAME MI_LOG_BREAK
#define MI_WARNING_PREFIX MI_LOG_WARNING_COLOR "[W] " MI_LOG_MODULE_NAME MI_LOG_BREAK
#define MI_INFO_PREFIX    MI_LOG_INFO_COLOR "[I] " MI_LOG_MODULE_NAME MI_LOG_BREAK
#define MI_DEBUG_PREFIX   MI_LOG_DEBUG_COLOR "[D] " MI_LOG_MODULE_NAME MI_LOG_BREAK

#ifndef MI_LOG_PRINTF
	#error "Not defined printf function."
#endif
 
#define MI_LOG_INTERNAL_ERROR(_fmt_, ...)                                       \
do {                                                                            \
    if (MI_LOG_LEVEL >= MI_LOG_LEVEL_ERROR)                                     \
    {                                                                           \
        MI_LOG_PRINTF(MI_ERROR_PREFIX _fmt_, ##__VA_ARGS__); \
    }                                                                           \
} while(0)

#define MI_LOG_INTERNAL_WARNING(_fmt_, ...)                                     \
do {                                                                            \
    if (MI_LOG_LEVEL >= MI_LOG_LEVEL_WARNING)                                   \
    {                                                                           \
        MI_LOG_PRINTF(MI_WARNING_PREFIX _fmt_, ##__VA_ARGS__); \
    }                                                                           \
} while(0)

#define MI_LOG_INTERNAL_INFO(_fmt_, ...)                                        \
do {                                                                            \
    if (MI_LOG_LEVEL >= MI_LOG_LEVEL_INFO)                                      \
    {                                                                           \
        MI_LOG_PRINTF(MI_INFO_PREFIX _fmt_, ##__VA_ARGS__); \
    }                                                                           \
} while(0)

#define MI_LOG_INTERNAL_DEBUG(_fmt_, ...)                                       \
do {                                                                            \
    if (MI_LOG_LEVEL >= MI_LOG_LEVEL_DEBUG)                                     \
    {                                                                           \
        MI_LOG_PRINTF(MI_DEBUG_PREFIX _fmt_, ##__VA_ARGS__);   \
    }                                                                           \
} while(0)

#define MI_LOG_INTERNAL_HEXDUMP_DEBUG(p_data, len)                              \
do {                                                                            \
    if (MI_LOG_LEVEL >= MI_LOG_LEVEL_ERROR)                                     \
    {                                                                           \
                                                                                \
    }                                                                           \
} while(0)
		
#endif // __MIBLE_LOG_INTERNAL__H__

