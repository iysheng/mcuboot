<!--
    -
    - Licensed to the Apache Software Foundation (ASF) under one
    - or more contributor license agreements.  See the NOTICE file
    - distributed with this work for additional information
    - regarding copyright ownership.  The ASF licenses this file
    - to you under the Apache License, Version 2.0 (the
    - "License"); you may not use this file except in compliance
    - with the License.  You may obtain a copy of the License at
    -
    -  http://www.apache.org/licenses/LICENSE-2.0
    -
    - Unless required by applicable law or agreed to in writing,
    - software distributed under the License is distributed on an
    - "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
    - KIND, either express or implied.  See the License for the
    - specific language governing permissions and limitations
    - under the License.
    -
-->

## Image signing

首先计算整个 image 的 hash 值，然后对这个 hash 值进行打签，在创建 image
的时候会使用 imgtool 对这个签名进行计算然后放在 image trailer 中

This signs the image by computing hash over the image, and then
signing that hash. Signature is computed by newt tool when it's
creating the image. This signature is placed in the image trailer.

bootloader 中包含公钥，在 image 启动之前会对 image 进行校验

The public key of this keypair must be included in the bootloader,
as it verifies it before allowing the image to run.

这个功能允许你使用多个签名.
This facility allows you to use multiple signing keys. This would
be useful when you want to prevent production units from booting
development images, but want development units to be able to boot
both production images and development images.

For an alternative solution when the public key(s) doesn't need to be
included in the bootloader, see the [design](design.md) document.

## Creating signing keys

首先，需要有一对 key 去打签。可以使用 openssl 创建这一对 key

First you need a keypair to use for signing. You can create
one with openssl command line tool.

rsa 2048 格式的密钥对
openssl genrsa -out image_sign.pem 2048

上述命令创建了一个包含有私钥和公钥的文件,可以用类给 image 打签
This created a file which contains both the private and public key,
and will be used when signing images.

Then you need to extract the public key from this to include it
in the bootloader. Bootloader need to keep key parsing minimal,
so it expects simple key format.

解析出 rsa 类型的公钥
openssl rsa -in image_sign.pem -pubout -out image_sign_pub.der -outform DER -RSAPublicKey_out

Now the public key is in file called image_sign_pub.der.

For ECDSA224 these commands are similar.

openssl ecparam -name secp224r1 -genkey -noout -out image_sign.pem
openssl ec -in image_sign.pem -pubout -outform DER -out image_sign_pub.der

And then the ECDSA256.
openssl ecparam -name prime256v1 -genkey -noout -out image_sign.pem
openssl ec -in image_sign.pem -pubout -outform DER -out image_sign_pub.der

## Creating a key package

将公钥转换为 C 语言格式
xxd -i image_sign_pub.der image_sign_pub.c.import

Then you need to create a package containing this key, or keys.

## Sample pkg.yml
This gets bootutil to turn on image signature validation.

    pkg.name: libs/mykeys
    pkg.deps:
        - "@apache-mynewt-core/boot/bootutil"

## Sample source file
创建一个 C 文件包含这个 key
This exports the keys.

    #include <bootutil/sign_key.h>

    #include "image_sign_pub.c.import"

    const struct bootutil_key bootutil_keys[] = {
        [0] = {
            .key = image_sign_pub_der,
            .len = &image_sign_pub_der_len,
        }
    };

    const int bootutil_key_cnt = sizeof(bootutil_keys) / sizeof(bootutil_keys[0]);

## Building the bootloader

Enable the BOOTUTIL_SIGN_RSA syscfg setting in your app or target syscfg.yml
file

    syscfg.vals:
        BOOTUTIL_SIGN_RSA: 1

After you've created the key package, you must include it in the build
for bootloader. So modify the pkg.yml for apps/boot to include it.

The syscfg variable to enable ECDSA224 is BOOTUTIL_SIGN_EC, and
BOOTUTIL_SIGN_EC256 for ECDS256.
