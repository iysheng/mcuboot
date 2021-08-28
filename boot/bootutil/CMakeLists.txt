#------------------------------------------------------------------------------
# Copyright (c) 2020, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
#------------------------------------------------------------------------------
# 创建 bootutil 的静态库文件
set(CMAKE_C_COMPILER /opt/arm_tools/gcc-arm-none-eabi-6_2-2016q4/bin/arm-none-eabi-gcc)
add_library(bootutil STATIC)

message("hello china")
target_include_directories(bootutil
    PUBLIC
        include
        /home/yangyongsheng/redwindow/red_20210629/tinycrypt/lib/include
    PRIVATE
        src
        rtthread/
)

# 添加源文件到 target
target_sources(bootutil
    PRIVATE
        src/boot_record.c
        src/bootutil_misc.c
        src/bootutil_public.c
        src/caps.c
        src/encrypted.c
        src/fault_injection_hardening.c
        src/fault_injection_hardening_delay_rng_mbedtls.c
        src/image_ec.c
        src/image_ec256.c
        src/image_ed25519.c
        src/image_rsa.c
        src/image_validate.c
        src/loader.c
        src/swap_misc.c
        src/swap_move.c
        src/swap_scratch.c
        src/tlv.c
        rtthread/bootutil_flash.c
)
