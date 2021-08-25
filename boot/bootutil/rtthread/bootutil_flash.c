/******************************************************************************
* File:             bootutil_flash.c
*
* Author:           iysheng@163.com  
* Created:          07/09/21 
*****************************************************************************/
#include <stdint.h>
#include "flash_map_backend/flash_map_backend.h"
#include "bootutil_priv.h"

/* 根据 image 的编号,以及 slot 的编号.获取 flash 的 id 信息 */
int flash_area_id_from_multi_image_slot(int image_index, int slot)
{
}

int flash_area_write(struct flash_area *fa, unsigned int offset, unsigned char *dst, unsigned int len)
{
}

/* 根据 flash area 的信息,返回 sector 的数量到 *num
 * 以及 flash area 的信息初始化到 sectors ???
 * */
int flash_area_to_sectors(struct flash_area *fa, int *num, boot_sector_t *sectors)
{
    /* 根据 flash area 信息,将包含的 sector 信息初始化到 sectors 指向的指针数组中,并且初始化 flash area 总的数量 */
}

/* 根据 flash id 以及 flash area 的指针,打开对应的 flash 设备 */
int flash_area_open(int idx, const struct flash_area **fa)
{
}

/* 根据 flash area 信息,以及在这个 flash area 的偏移读取指定长度 len 的数据到 dst 指向的内存空间 */
int flash_area_read(const struct flash_area *fa, unsigned int offset, unsigned char *dst, unsigned int len)
{
}

int flash_area_align(const struct flash_area *fa)
{
    return 8;
}

int flash_area_close(const struct flash_area *fa)
{
}

int flash_area_id_from_image_slot(int slot)
{
}

uint8_t flash_area_erased_val(const struct flash_area *fa)
{
    return 0xff;
}

int flash_area_erase(struct flash_area *fa, unsigned int off, unsigned int len)
{
}

