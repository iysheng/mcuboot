/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2016-2019 Linaro LTD
 * Copyright (c) 2016-2019 JUUL Labs
 * Copyright (c) 2019-2021 Arm Limited
 *
 * Original license:
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef H_IMAGE_
#define H_IMAGE_

#include <inttypes.h>
#include <stdbool.h>
#include "bootutil/fault_injection_hardening.h"

#ifdef __cplusplus
extern "C" {
#endif

struct flash_area;

#define IMAGE_MAGIC                 0x96f3b83d
#define IMAGE_MAGIC_V1              0x96f3b83c
#define IMAGE_MAGIC_NONE            0xffffffff
#define IMAGE_TLV_INFO_MAGIC        0x6907
#define IMAGE_TLV_PROT_INFO_MAGIC   0x6908

#define IMAGE_HEADER_SIZE           32

/*
 * Image header flags.
 */
#define IMAGE_F_PIC                      0x00000001 /* Not supported. */
#define IMAGE_F_ENCRYPTED_AES128         0x00000004 /* Encrypted using AES128. */
#define IMAGE_F_ENCRYPTED_AES256         0x00000008 /* Encrypted using AES256. */
#define IMAGE_F_NON_BOOTABLE             0x00000010 /* Split image app. */
/*
 * Indicates that this image should be loaded into RAM instead of run
 * directly from flash.  The address to load should be in the
 * ih_load_addr field of the header.
 */
#define IMAGE_F_RAM_LOAD                 0x00000020

/*
 * Indicates that ih_load_addr stores information on flash/ROM address the
 * image has been built for.
 */
#define IMAGE_F_ROM_FIXED                0x00000100

/*
 * ECSDA224 is with NIST P-224
 * ECSDA256 is with NIST P-256
 */

/*
 * Image trailer TLV types.
 *
 * Signature is generated by computing signature over the image hash.
 * Currently the only image hash type is SHA256.
 *
 * Signature comes in the form of 2 TLVs.
 *   1st on identifies the public key which should be used to verify it.
 *   2nd one is the actual signature.
 */
#define IMAGE_TLV_KEYHASH           0x01   /* hash of the public key */
#define IMAGE_TLV_PUBKEY            0x02   /* public key */
#define IMAGE_TLV_SHA256            0x10   /* SHA256 of image hdr and body */
#define IMAGE_TLV_RSA2048_PSS       0x20   /* RSA2048 of hash output */
#define IMAGE_TLV_ECDSA224          0x21   /* ECDSA of hash output */
#define IMAGE_TLV_ECDSA256          0x22   /* ECDSA of hash output */
#define IMAGE_TLV_RSA3072_PSS       0x23   /* RSA3072 of hash output */
#define IMAGE_TLV_ED25519           0x24   /* ed25519 of hash output */
#define IMAGE_TLV_ENC_RSA2048       0x30   /* Key encrypted with RSA-OAEP-2048 */
#define IMAGE_TLV_ENC_KW            0x31   /* Key encrypted with AES-KW 128 or 256*/
#define IMAGE_TLV_ENC_EC256         0x32   /* Key encrypted with ECIES-EC256 */
#define IMAGE_TLV_ENC_X25519        0x33   /* Key encrypted with ECIES-X25519 */
#define IMAGE_TLV_DEPENDENCY        0x40   /* Image depends on other image */
#define IMAGE_TLV_SEC_CNT           0x50   /* security counter */
#define IMAGE_TLV_BOOT_RECORD       0x60   /* measured boot record */
					   /*
					    * vendor reserved TLVs at xxA0-xxFF,
					    * where xx denotes the upper byte
					    * range.  Examples:
					    * 0x00a0 - 0x00ff
					    * 0x01a0 - 0x01ff
					    * 0x02a0 - 0x02ff
					    * ...
					    * 0xffa0 - 0xfffe
					    */
#define IMAGE_TLV_ANY               0xffff /* Used to iterate over all TLV */

/* 镜像的版本号 */
struct image_version {
    uint8_t iv_major;
    uint8_t iv_minor;
    uint16_t iv_revision;
    uint32_t iv_build_num;
};

struct image_dependency {
    uint8_t image_id;                       /* Image index (from 0) */
    uint8_t _pad1;
    uint16_t _pad2;
    struct image_version image_min_version; /* Indicates at minimum which
                                             * version of firmware must be
                                             * available to satisfy compliance
                                             */
};

/** Image header.  All fields are in little endian byte order. */
/* 镜像的头部信息 */
struct image_header {
    /* image_header 头部的魔数 #define IMAGE_MAGIC                 0x96f3b83d */
    uint32_t ih_magic; 
    uint32_t ih_load_addr;
    /* image header 的大小,也指示了镜像的偏移长度,并且提供了向后兼容的支持 */
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
    /* 镜像 tlv 的魔数, == IMAGE_TLV_PROT_INFO_MAGIC */
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

#define IS_ENCRYPTED(hdr) (((hdr)->ih_flags && IMAGE_F_ENCRYPTED_AES128) \
                        || ((hdr)->ih_flags && IMAGE_F_ENCRYPTED_AES256))
#define MUST_DECRYPT(fap, idx, hdr) \
    ((fap)->fa_id == FLASH_AREA_IMAGE_SECONDARY(idx) && IS_ENCRYPTED(hdr))

_Static_assert(sizeof(struct image_header) == IMAGE_HEADER_SIZE,
               "struct image_header not required size");

struct enc_key_data;
fih_int bootutil_img_validate(struct enc_key_data *enc_state, int image_index,
                              struct image_header *hdr,
                              const struct flash_area *fap,
                              uint8_t *tmp_buf, uint32_t tmp_buf_sz,
                              uint8_t *seed, int seed_len, uint8_t *out_hash);

struct image_tlv_iter {
    const struct image_header *hdr;
    const struct flash_area *fap;
    uint16_t type;
    bool prot;
    uint32_t prot_end;
    uint32_t tlv_off;
    uint32_t tlv_end;
};

int bootutil_tlv_iter_begin(struct image_tlv_iter *it,
                            const struct image_header *hdr,
                            const struct flash_area *fap, uint16_t type,
                            bool prot);
int bootutil_tlv_iter_next(struct image_tlv_iter *it, uint32_t *off,
                           uint16_t *len, uint16_t *type);

int32_t bootutil_get_img_security_cnt(struct image_header *hdr,
                                      const struct flash_area *fap,
                                      uint32_t *security_cnt);

#ifdef __cplusplus
}
#endif

#endif
