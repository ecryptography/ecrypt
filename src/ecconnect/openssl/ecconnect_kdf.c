/*
 * Copyright (c) 2019 Cossack Labs Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ecconnect/ecconnect_kdf.h"

#include <limits.h>

#include <openssl/evp.h>

ecconnect_status_t ecconnect_pbkdf2_sha256(const uint8_t* passphrase,
                                   size_t passphrase_length,
                                   const uint8_t* salt,
                                   size_t salt_length,
                                   size_t iterations,
                                   uint8_t* key,
                                   size_t key_length)
{
    int res;

    ECCONNECT_CHECK_PARAM(passphrase != NULL);
    ECCONNECT_CHECK_PARAM(passphrase_length > 0);
    ECCONNECT_CHECK_PARAM(passphrase_length <= INT_MAX);
    if (!salt) {
        ECCONNECT_CHECK_PARAM(salt_length == 0);
    }
    ECCONNECT_CHECK_PARAM(salt_length <= INT_MAX);
    ECCONNECT_CHECK_PARAM(iterations > 0);
    ECCONNECT_CHECK_PARAM(iterations <= INT_MAX);
    ECCONNECT_CHECK_PARAM(key != NULL);
    ECCONNECT_CHECK_PARAM(key_length > 0);
    ECCONNECT_CHECK_PARAM(key_length <= INT_MAX);

    res = PKCS5_PBKDF2_HMAC((const char*)passphrase,
                            (int)passphrase_length,
                            salt,
                            (int)salt_length,
                            (int)iterations,
                            EVP_sha256(),
                            (int)key_length,
                            key);

    return (res == 1) ? ECCONNECT_SUCCESS : ECCONNECT_FAIL;
}
