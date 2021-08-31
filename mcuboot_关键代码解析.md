### mcuboot 关键代码解析
#### 关键结构体
~~头文件 :fault_injection_hardening.h~~
Fault injection mitigation library(故障注入缓解库)
``` C
typedef volatile struct {
    volatile int val; /* 原始数据 */
    volatile int msk; /* 原始数据和 #define _FIH_MASK_VALUE 0xBEEF 异或 */
} fih_int;
```

``` C
/** Private state maintained during boot. */
struct boot_loader_state {
    struct {
        struct image_header hdr;
        const struct flash_area *area;
        boot_sector_t *sectors;
        uint32_t num_sectors;
    } imgs[BOOT_IMAGE_NUMBER][BOOT_NUM_SLOTS];

#if MCUBOOT_SWAP_USING_SCRATCH
    struct {
        const struct flash_area *area;
        boot_sector_t *sectors;
        uint32_t num_sectors;
    } scratch;
#endif

    uint8_t swap_type[BOOT_IMAGE_NUMBER];
    uint32_t write_sz;

#if defined(MCUBOOT_ENC_IMAGES)
    struct enc_key_data enc[BOOT_IMAGE_NUMBER][BOOT_NUM_SLOTS];
#endif

#if (BOOT_IMAGE_NUMBER > 1)
    uint8_t curr_img_idx;
#endif
};

/** Image header.  All fields are in little endian byte order. */
/* 镜像的头部信息 */
struct image_header {
    /* image_header 头部的魔数 */
    uint32_t ih_magic; 
    uint32_t ih_load_addr;
    /* image header 的大小 */
    uint16_t ih_hdr_size;           /* Size of image header (bytes). */
    /* 受保护的 image header 的 tlv 的大小 */
    uint16_t ih_protect_tlv_size;   /* Size of protected TLV area (bytes). */
    /* 镜像的大小 */
    uint32_t ih_img_size;           /* Does not include header. */
    /* image header 的 flags */
    uint32_t ih_flags;              /* IMAGE_F_[...]. */
    /* 镜像的版本 */
    struct image_version ih_ver;
    uint32_t _pad1;
};

/** Image TLV header.  All fields in little endian. */
/* 镜像中 tlv indo 的头部信息
 * tlv 是 type-length-value
 * */
struct image_tlv_info {
    /* 镜像 tlv 的头部 */
    uint16_t it_magic;
    /* 镜像 tlv 区整个的区域大小,包含 tlv_info 的头部 */
    uint16_t it_tlv_tot;  /* size of TLV area (including tlv_info header) */
};

/** Image trailer TLV format. All fields in little endian. */
/* tlv 结构体 */
struct image_tlv {
    uint16_t it_type;   /* IMAGE_TLV_[...]. */
    uint16_t it_len;    /* Data length (not including TLV header). */
};

/**
 * A response object provided by the boot loader code; indicates where to jump
 * to execute the main image.
 */
struct boot_rsp {
    /** A pointer to the header of the image to be executed. */
    const struct image_header *br_hdr;

    /**
     * The flash offset of the image to execute.  Indicates the position of
     * the image header within its flash device.
     */
    uint8_t br_flash_dev_id;
    uint32_t br_image_off;
};

```
#### 关键概念
1. Image Format
    * struct image_version
    * struct image_header
    * struct image_tlv_info 根据魔数还支持一种收保护的 tlv 对应的魔数是 `IMAGE_TLV_PROT_INFO_MAGIC` 而常规的 tlv 对应的魔数是 `IMAGE_TLV_INFO_MAGIC`
    * struct image_tlv
2. Flash Map: 一个 flash 设备根据他的 flash map 可以被划分为多个 flash area.
   
* Flash Area : 一个 flash area 对应会有一个数字化的 ID
  
3. mcuboot 尝试标准化两件事情:
    1. 安全启动功能: bootloader 只会启动经过加密认证的 image
    2. 标准化 flash 布局: 定义嵌入式系统中 flash map 的通用方法,也就是定义 flash 数据保存布局的方法
    * 此外 mcuboot 还支持加密.解密固件, 升级,容错(出现意外时可以及时恢复之前状态)
4. 升级的策略
    * images: 独立的可以执行的固件
    * slots: 每一个 image 都可以保存在 primary slot 或者 secondary slot, The primary slot 就是镜像执行的地方. Flash 会根据 flash map 划分为多个 image area,每一个 image area 都会包含两个 slots: primary slot 和 secondary slot.
    * mcuboot 支持多种方式从 secondary slot 更新固件到 primary slot.
        1. overwrite mode: `MCUBOOT_OVERWRITE_ONLY` 这种覆盖模式下. primary slot 的内容会被擦除掉,secondary slot 的内容会被复制到 prmary slot
        2. swap mode: swap 模式又可以分为多种方法.如果安装的新的 primary slot 的镜像出问题了,那么可以恢复到更新之前的镜像.
            * `MCUBOOT_SWAP_USING_MOVE` 这种模式下,会首先将 primary slot 的镜像移动到一个额外的 flash 空间,然后再将 secondary slot 的内容移动到 primary slot.
            * `MCUBOOT_SWAP_USING_SCRATCH` 这种模式下,会需要先分配出一小块区域,来作为 primary slot 和 secondary slot 之间交换信息的中转站,因为会频繁的擦写这块 scratch 区域,所以还要考虑磨损均衡.
5. 加密
    * mcuboot 自己不提供加密方法,相反期待你使用现有的加密方法,默认的支持的加密方法有 `Tinycrypt` 和 `Mbed TLS` 对应开启的宏是 `MCUBOOT_USE_TINYCRYPT` 和 `MCUBOOT_USE_MBED_TLS`.
    * `MCUBOOT_VALIDATE_PRIMARY_SLOT` 这个宏用来控制是否每次启动上电都校验镜像
6. MCUboot image
    * MCUboot image 是在原始的 image 的基础上添加一个头部以及一些尾部. `imgtool.py` 这个脚本是用来生成这个镜像的.
    * 头部包含的是`struct image_header` 其中包含了 `struct image_version`.
    * 镜像的尾部信息中包含的是 TLV(type length value).这里面包含了 image 是如何加密的以及打签的.用户也可以自己添加自己私有的元数据信息.使用 tlv 的好处是避免了大量冗余空间的浪费.以及对终端用户提供信息不灵活的弊端.因为 tlv 包含的有必要的 type 信息.
        * `struct image_tlv_info`  tlv 的头部
        * `struct image_tlv` tlv 实体的抽象,特别地 tlv 的类型是以`IMAGE_TLV_(...)` 格式定义的
7. Image Trailers
   
    * `Image Trailers` 信息保存在 slot 的尾部,这里包含了存储的 image 的元数据,以及是否需要更新操作.简单来说,上电的时候, 会检查 `swap status` 来确认是否正在更新以及恢复更新.`swap info` `copy done` 以及 `image ok`用来检查是否需要更新的.
```
     0                   1                   2                   3
     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    ~                                                               ~
    ~    Swap status (BOOT_MAX_IMG_SECTORS * min-write-size * 3)    ~
    ~                                                               ~
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                 Encryption key 0 (16 octets) [*]              |
    |                                                               |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                 Encryption key 1 (16 octets) [*]              |
    |                                                               |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                      Swap size (4 octets)                     |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |   Swap info   |           0xff padding (7 octets)             |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |   Copy done   |           0xff padding (7 octets)             |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |   Image OK    |           0xff padding (7 octets)             |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                       MAGIC (16 octets)                       |
    |                                                               |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```
其中 `*` 的内容表示只有在加密开启的时候才存在.加密开启使用宏`MCUBOOT_ENC_IMAGES`
紧挨着这个记录的区域是下一个 flash area 的开始. `min-write-size` 是 flash 硬件的一个属性.如果 flash 可以写任意地址,那么这个值就是 1,如果只能写偶数地址那么这个值就是 2 以此类推.
`image trailer` 包含有如下内容:
    1. swap status:一些记录用来跟踪 image 的 swap 阶段.在更新固件的时候,这部分区域会随着更新的过程不断动态变化,这样可以实时跟踪更新的进度,如果系统更新到中途出现问题,下次启动的时候还可以继续更新.`BOOT_MAX_IMG_SECTORS` 的值是 image 占有的 sector 的数量.这个值默认是 128.
    2. encryption keys: key-encrypting keys. 这些是涉及到加解秘的 key.
    3. swap size: 在开始 swap 之前,总的需要 swap 的数据大小(image 的大小 + tlvs)需要写到这个区域.
    4. swap info: 一个字节的数据,包含了如下信息 
        * swap type: bit[0-3] `BOOT_SWAP_TYPE_TEST` 或者 `BOOT_SWAP_TYPE_PERM` 或者 `BOOT_SWAP_TYPE_REVERT`
        * image number: bit[4-7] 在只有一个 image 的时候,这个区域的值是总是 0,当有多个镜像的时候,这个数值来决定哪个镜像来进行 swap.以及在从更新中断中恢复的时候决定继续和哪一个 image 进行 swap.
    5. copy done: 决定这个 image 是否完成了 copy. 0x01 表示已完成, 0xff 表示未完成
    6. image ok: 决定这个 image 是否被检验通过, 0x01 检验通过, 0xff 未检验通过
    7. MAGIC: 16 个字节
    ``` C
    const uint32_t boot_img_magic[4] = {
        0xf395c277,
        0x7fefd260,
        0x0f505235,
        0x8079b62c,
    };
    ```
    开始阶段,会根据 `image trailer` 的 swap info 决定 swap 的类型.针对新的 swap, mcuboot 必须检查这些集合,来决定怎么进行 swap.并且很难仅仅通过查看 image trailers 来跟踪设备的状态.更好的方法是将所有可能的 trailer states 的状态映射到一个表中.对应的就是 `swap status` 区域.如果需要固件更新的时候,状态的迁移 `BOOT_SWAP_TYPE_TEST->BOOT_SWAP_TYPE_PERM->BOOT_SWAP_TYPE_REVERT`.
    如果不需要 swap,即只有一个  image,那么就涉及到第 4 种状态,这时候会有 3 种结果.`BOOT_SWAP_TYPE_NONE` 或者 `BOOT_SWAP_TYPE_FAIL` 或者 `BOOT_SWAP_TYPE_PANIC`. 如果没有错误发生,那么 mcuboot 就会尝试直接启动 primary slot 的镜像,结果就是 `BOOT_SWAP_TYPE_NONE`.如果 primary slot 的 image 无效,那么返回 `BOOT_SWAP_TYPE_FAIL`,如果在启动过程中发生错误,那么返回的是 `BOOT_SWAP_TYPE_PANIC`. 如果返回值是 `BOOT_SWAP_TYPE_FAIL` 或者 `BOOT_SWAP_TYPE_PANIC`, mcuboot 就会一直卡在这里而不会启动未经过检查的镜像.
    特别地,如果在 swap 过程中,发现在 secondary slot 校验 image 出错了,这时候就会进入到状态 4,并且会标记 primary slot OK 的状态,避免无效的 swap.
8. high level 操作
    1. 检查 swap status 的状态,是否在恢复一个中断的 swap
        1. 是: 继续完成 swap, 跳转到 3
        2. 否: 跳转到 2
    2. 检查 image trailers: 是否需要 swap
        1. 检查 swap 的 image 是否有效,
            a. 有效, swap 然后更新状态到 image trailer 执行 3
            b. 无效, 擦除分区,更新 image trailer 的状态 然后执行 3
        2. 跳转到 3
    3. 启动在 primary slot 的镜像
9. RAM loading 模式, 目前分析移植到 ART-Pi 需要这个模式
``` C
#define IMAGE_EXECUTABLE_RAM_START    <area_base_addr>
#define IMAGE_EXECUTABLE_RAM_SIZE     <area_size_in_bytes>
```
#### 关键函数
/*  启动函数 */
``` C
fih_int boot_go(struct boot_rsp *rsp)
    fih_int context_boot_go(struct boot_loader_state *state, struct boot_rsp *rsp)
```


``` C
/*  进行跳转,典型的跳转函数示例 */
static void do_boot(struct boot_rsp \*rsp)
/* 获取 flash 的基地址 */
    rc = flash_device_base(rsp->br_flash_dev_id, &flash_base);
    /* 获取向量表首地址
     * 有效的 image 的首地址
     * */
    vt = (struct arm_vector_table *)(flash_base +
                                     rsp->br_image_off +
                                     rsp->br_hdr->ih_hdr_size);
    /* 设置 SCB 控制器 */
    SCB->VTOR = (uint32_t)__vector_relay_table;
    /* 设置栈指针 */
    __set_MSP(vt->msp);
    /* 执行 img 的 reset 函数 */
    ((void (*)(void))vt->reset)();
```
#### 移植笔记
* stm32h750x flash 是 128KB , SRAM 512KB
其中, sram 布局为:
![](figures/sram.png)
其中 DTCM RAM 是 cortex-M7 专有的,映射在 0x20000000- 起始的这段内存区
flash 布局为: 
![](figures/flash.png)
在 flash 的 memory mapped 模式,最大映射的空间大小不超过 256MB
---
w25q64jv 容量为 8MB,page 大小为 256 B, sector 大小为 4KB,block 大小为 128KB,其中不同的擦除指令擦除去 sector 大小不一样.
|擦除指令|擦除空间大小|
|---|---|
|0x20|4KB|
|0x52|32KB|
|0xD8|64KB|
一共有 16 个 blcok
