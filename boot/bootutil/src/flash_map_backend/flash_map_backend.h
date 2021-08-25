/******************************************************************************
* File:             flash_map_backend.h
*
* Author:           iysheng@163.com
* Created:          07/08/21
* Description:
*****************************************************************************/
#ifndef __FLASH_MAP_BACKEND_H__
#define __FLASH_MAP_BACKEND_H__

#include <stdint.h>

struct flash_area {
    uint8_t  fa_id;         /** The slot/scratch identification */
    uint8_t  fa_device_id;  /** The device id (usually there's only one) */
    uint16_t pad16;
    uint32_t fa_off;        /** The flash offset from the beginning */
    uint32_t fa_size;       /** The size of this sector */
};
#if 0
int flash_area_to_sectors(struct flash_area *fa, int *num, boot_sector_t *sectors);
#else
int flash_area_id_from_multi_image_slot(int image_index, int slot);
int flash_area_write(struct flash_area *fa, unsigned int offset, unsigned char *dst, unsigned int len);
int flash_area_open(int idx, const struct flash_area **fa);
int flash_area_read(const struct flash_area *fa, unsigned int offset, unsigned char *dst, unsigned int len);
int flash_area_align(const struct flash_area *fa);
int flash_area_close(const struct flash_area *fa);
int flash_area_id_from_image_slot(int slot);
uint8_t flash_area_erased_val(const struct flash_area *fa);
int flash_area_erase(struct flash_area *fa, unsigned int off, unsigned int len);
#endif

#endif
