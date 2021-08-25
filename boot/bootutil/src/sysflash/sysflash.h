/******************************************************************************
* File:             sysflash.h
*
* Author:           iysheng@163.com  
* Created:          07/08/21 
* Description:      
*****************************************************************************/
#ifndef __SYS_FLASH_H__
#define __SYS_FLASH_H__

#include <stdint.h>

#define FLASH_AREA_IMAGE_SCRATCH	0
#define FLASH_AREA_IMAGE_PRIMARY(id) ((id == 0) ? 1 : 255)
#define FLASH_AREA_IMAGE_SECONDARY(id) ((id == 0) ? 2 : 255)

#endif
