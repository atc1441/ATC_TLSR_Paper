/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __MI_DFU_FLASH_H__
#define __MI_DFU_FLASH_H__

/* Includes ------------------------------------------------------------------*/

#include "mible_type.h"

/* Exported functions --------------------------------------------------------*/

mible_status_t mible_dfu_flash_init(uint32_t flash_start, uint32_t flash_size);
mible_status_t mible_dfu_flash_read(void * buffer, uint32_t length, uint32_t offset);
mible_status_t mible_dfu_flash_write(void * buffer, uint32_t length, uint32_t offset);

#endif
