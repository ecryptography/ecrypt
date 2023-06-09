/*
 * Copyright (c) 2015 Cossack Labs Limited
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

#include "ecrypt/sym_enc_message.h"

#include <string.h>

#include <ecconnect/ecconnect.h>

#include "ecrypt/secure_cell_alg.h"
#include "ecrypt/ecrypt_portable_endian.h"

#define ECRYPT_SYM_KDF_KEY_LABEL "Ecrypt secure cell message key"
#define ECRYPT_SYM_KDF_IV_LABEL "Ecrypt secure cell message iv"

ecrypt_status_t ecrypt_sym_kdf(const uint8_t* master_key,
                               const size_t master_key_length,
                               const char* label,
                               const uint8_t* context,
                               const size_t context_length,
                               const uint8_t* context2,
                               const size_t context2_length,
                               uint8_t* key,
                               size_t key_length)
{
    ECRYPT_CHECK_PARAM(master_key != NULL && master_key_length != 0);
    ECRYPT_CHECK_PARAM(context != NULL && context_length != 0);
    ecconnect_kdf_context_buf_t ctx[2] = {{context, context_length}, {context2, context2_length}};
    ECRYPT_CHECK(ecconnect_kdf(master_key,
                           master_key_length,
                           label,
                           ctx,
                           (context2 == NULL || context2_length == 0) ? 1 : 2,
                           key,
                           key_length)
                 == ECRYPT_SUCCESS);
    return ECRYPT_SUCCESS;
}

ecrypt_status_t ecrypt_auth_sym_plain_encrypt(uint32_t alg,
                                              const uint8_t* key,
                                              const size_t key_length,
                                              const uint8_t* iv,
                                              const size_t iv_length,
                                              const uint8_t* aad,
                                              const size_t aad_length,
                                              const uint8_t* message,
                                              const size_t message_length,
                                              uint8_t* encrypted_message,
                                              size_t* encrypted_message_length,
                                              uint8_t* auth_tag,
                                              uint32_t* auth_tag_length)
{
    size_t auth_tag_length_ = *auth_tag_length;
    ecconnect_sym_ctx_t* ctx = ecconnect_sym_aead_encrypt_create(alg, key, key_length, NULL, 0, iv, iv_length);
    ECRYPT_CHECK(ctx != NULL);
    if (aad != NULL || aad_length != 0) {
        ECRYPT_CHECK__(ecconnect_sym_aead_encrypt_aad(ctx, aad, aad_length) == ECRYPT_SUCCESS,
                       ecconnect_sym_aead_encrypt_destroy(ctx);
                       return ECRYPT_FAIL);
    }
    ECRYPT_CHECK__(ecconnect_sym_aead_encrypt_update(ctx, message, message_length, encrypted_message, encrypted_message_length)
                       == ECRYPT_SUCCESS,
                   ecconnect_sym_aead_encrypt_destroy(ctx);
                   return ECRYPT_FAIL);
    ECRYPT_CHECK__(ecconnect_sym_aead_encrypt_final(ctx, auth_tag, &auth_tag_length_) == ECRYPT_SUCCESS,
                   ecconnect_sym_aead_encrypt_destroy(ctx);
                   return ECRYPT_FAIL);
    ecconnect_sym_aead_encrypt_destroy(ctx);
    if (auth_tag_length_ > UINT32_MAX) {
        return ECRYPT_INVALID_PARAMETER;
    }
    *auth_tag_length = (uint32_t)auth_tag_length_;
    return ECRYPT_SUCCESS;
}

ecrypt_status_t ecrypt_auth_sym_plain_decrypt(uint32_t alg,
                                              const uint8_t* key,
                                              const size_t key_length,
                                              const uint8_t* iv,
                                              const size_t iv_length,
                                              const uint8_t* aad,
                                              const size_t aad_length,
                                              const uint8_t* encrypted_message,
                                              const size_t encrypted_message_length,
                                              uint8_t* message,
                                              size_t* message_length,
                                              const uint8_t* auth_tag,
                                              const size_t auth_tag_length)
{
    ecconnect_sym_ctx_t* ctx = ecconnect_sym_aead_decrypt_create(alg, key, key_length, NULL, 0, iv, iv_length);
    ECRYPT_CHECK(ctx != NULL)
    if (aad != NULL || aad_length != 0) {
        ECRYPT_CHECK__(ecconnect_sym_aead_decrypt_aad(ctx, aad, aad_length) == ECRYPT_SUCCESS,
                       ecconnect_sym_aead_decrypt_destroy(ctx);
                       return ECRYPT_FAIL);
    }
    ECRYPT_CHECK__(ecconnect_sym_aead_decrypt_update(ctx, encrypted_message, encrypted_message_length, message, message_length)
                       == ECRYPT_SUCCESS,
                   ecconnect_sym_aead_decrypt_destroy(ctx);
                   return ECRYPT_FAIL);
    ECRYPT_CHECK__(ecconnect_sym_aead_decrypt_final(ctx, auth_tag, auth_tag_length) == ECRYPT_SUCCESS,
                   ecconnect_sym_aead_decrypt_destroy(ctx);
                   return ECRYPT_FAIL);
    ecconnect_sym_aead_decrypt_destroy(ctx);
    return ECRYPT_SUCCESS;
}

ecrypt_status_t ecrypt_sym_plain_encrypt(uint32_t alg,
                                         const uint8_t* key,
                                         const size_t key_length,
                                         const uint8_t* iv,
                                         const size_t iv_length,
                                         const uint8_t* message,
                                         const size_t message_length,
                                         uint8_t* encrypted_message,
                                         size_t* encrypted_message_length)
{
    ecconnect_sym_ctx_t* ctx = ecconnect_sym_encrypt_create(alg, key, key_length, NULL, 0, iv, iv_length);
    ECRYPT_CHECK__(ctx, return ECRYPT_NO_MEMORY);
    size_t add_length = (*encrypted_message_length);
    ecrypt_status_t res = ecconnect_sym_encrypt_update(ctx,
                                                   message,
                                                   message_length,
                                                   encrypted_message,
                                                   encrypted_message_length);
    ECRYPT_CHECK__(ECRYPT_SUCCESS == res, ecconnect_sym_encrypt_destroy(ctx); return res);
    add_length -= (*encrypted_message_length);
    res = ecconnect_sym_encrypt_final(ctx, encrypted_message + (*encrypted_message_length), &add_length);
    ECRYPT_CHECK__(res == ECRYPT_SUCCESS, ecconnect_sym_encrypt_destroy(ctx);
                   (*encrypted_message_length) += add_length;
                   return res);
    (*encrypted_message_length) += add_length;
    ecconnect_sym_encrypt_destroy(ctx);
    return ECRYPT_SUCCESS;
}

ecrypt_status_t ecrypt_sym_plain_decrypt(uint32_t alg,
                                         const uint8_t* key,
                                         const size_t key_length,
                                         const uint8_t* iv,
                                         const size_t iv_length,
                                         const uint8_t* encrypted_message,
                                         const size_t encrypted_message_length,
                                         uint8_t* message,
                                         size_t* message_length)
{
    ecconnect_sym_ctx_t* ctx = ecconnect_sym_decrypt_create(alg, key, key_length, NULL, 0, iv, iv_length);
    ECRYPT_CHECK(ctx != NULL);
    size_t add_length = (*message_length);
    ecrypt_status_t res = ecconnect_sym_decrypt_update(ctx,
                                                   encrypted_message,
                                                   encrypted_message_length,
                                                   message,
                                                   message_length);
    ECRYPT_CHECK__(ECRYPT_SUCCESS == res, ecconnect_sym_decrypt_destroy(ctx); return res);
    add_length -= (*message_length);
    res = ecconnect_sym_decrypt_final(ctx, message + (*message_length), &add_length);
    ECRYPT_CHECK__(res == ECRYPT_SUCCESS, ecconnect_sym_decrypt_destroy(ctx);
                   (*message_length) += add_length;
                   return res);
    (*message_length) += add_length;
    ecconnect_sym_decrypt_destroy(ctx);
    return ECRYPT_SUCCESS;
}

static inline size_t default_auth_token_size(void)
{
    return ecrypt_scell_auth_token_key_min_size + ECRYPT_AUTH_SYM_IV_LENGTH
           + ECRYPT_AUTH_SYM_AUTH_TAG_LENGTH;
}

ecrypt_status_t ecrypt_auth_sym_kdf_context(uint32_t message_length,
                                            uint8_t* kdf_context,
                                            size_t* kdf_context_length)
{
    if (*kdf_context_length < sizeof(uint32_t)) {
        *kdf_context_length = sizeof(uint32_t);
        return ECRYPT_BUFFER_TOO_SMALL;
    }
    stream_write_uint32LE(kdf_context, message_length);
    *kdf_context_length = sizeof(uint32_t);
    return ECRYPT_SUCCESS;
}

#ifdef SCELL_COMPAT
/*
 * Ecrypt 0.9.6 incorrectly used 64-bit message length for this field.
 */
static ecrypt_status_t ecrypt_auth_sym_kdf_context_compat(uint32_t message_length,
                                                          uint8_t* kdf_context,
                                                          size_t* kdf_context_length)
{
    if (*kdf_context_length < sizeof(uint64_t)) {
        *kdf_context_length = sizeof(uint64_t);
        return ECRYPT_BUFFER_TOO_SMALL;
    }
    stream_write_uint64LE(kdf_context, message_length);
    *kdf_context_length = sizeof(uint64_t);
    return ECRYPT_SUCCESS;
}
#endif

ecrypt_status_t ecrypt_auth_sym_derive_encryption_key(uint32_t ecconnect_alg,
                                                      const uint8_t* key,
                                                      size_t key_length,
                                                      const uint8_t* kdf_context,
                                                      size_t kdf_context_length,
                                                      const uint8_t* user_context,
                                                      size_t user_context_length,
                                                      uint8_t* derived_key,
                                                      size_t* derived_key_length)
{
    size_t required_length = ecconnect_alg_key_length(ecconnect_alg);
    switch (required_length) {
    case ECCONNECT_SYM_256_KEY_LENGTH / 8:
    case ECCONNECT_SYM_192_KEY_LENGTH / 8:
    case ECCONNECT_SYM_128_KEY_LENGTH / 8:
        break;
    default:
        return ECRYPT_FAIL;
    }
    /* Internal buffer must have suitable size */
    if (*derived_key_length < required_length) {
        return ECRYPT_FAIL;
    }
    *derived_key_length = required_length;
    /*
     * ECCONNECT_SYM_NOKDF means ecconnect KDF in this context.
     * This is the only KDF allowed for master key API.
     */
    switch (ecconnect_alg_kdf(ecconnect_alg)) {
    case ECCONNECT_SYM_NOKDF: {
        return ecrypt_sym_kdf(key,
                              key_length,
                              ECRYPT_SYM_KDF_KEY_LABEL,
                              kdf_context,
                              kdf_context_length,
                              user_context,
                              user_context_length,
                              derived_key,
                              *derived_key_length);
    }
    default:
        return ECRYPT_FAIL;
    }
}

ecrypt_status_t ecrypt_auth_sym_encrypt_message_(const uint8_t* key,
                                                 size_t key_length,
                                                 const uint8_t* message,
                                                 size_t message_length,
                                                 const uint8_t* user_context,
                                                 size_t user_context_length,
                                                 uint8_t* auth_token,
                                                 size_t* auth_token_length,
                                                 uint8_t* encrypted_message,
                                                 size_t* encrypted_message_length)
{
    ecrypt_status_t res = ECRYPT_FAIL;
    uint8_t kdf_context[ECRYPT_AUTH_SYM_MAX_KDF_CONTEXT_LENGTH] = {0};
    uint8_t iv[ECRYPT_AUTH_SYM_IV_LENGTH] = {0};
    uint8_t auth_tag[ECRYPT_AUTH_SYM_AUTH_TAG_LENGTH] = {0};
    uint8_t derived_key[ECRYPT_AUTH_SYM_KEY_LENGTH / 8] = {0};
    size_t kdf_context_length = sizeof(kdf_context);
    size_t derived_key_length = sizeof(derived_key);
    size_t auth_token_real_length = 0;
    struct ecrypt_scell_auth_token_key hdr;

    /* Message length is currently stored as 32-bit integer, sorry */
    if (message_length > UINT32_MAX) {
        return ECRYPT_INVALID_PARAMETER;
    }

    memset(&hdr, 0, sizeof(hdr));
    hdr.alg = ECRYPT_AUTH_SYM_ALG;
    hdr.iv = iv;
    hdr.iv_length = sizeof(iv);
    hdr.auth_tag = auth_tag;
    hdr.auth_tag_length = sizeof(auth_tag);
    hdr.message_length = (uint32_t)message_length;

    res = ecrypt_auth_sym_kdf_context(hdr.message_length, kdf_context, &kdf_context_length);
    if (res != ECRYPT_SUCCESS) {
        goto error;
    }
    res = ecrypt_auth_sym_derive_encryption_key(hdr.alg,
                                                key,
                                                key_length,
                                                kdf_context,
                                                kdf_context_length,
                                                user_context,
                                                user_context_length,
                                                derived_key,
                                                &derived_key_length);
    if (res != ECRYPT_SUCCESS) {
        goto error;
    }

    res = ecconnect_rand(iv, sizeof(iv));
    if (res != ECRYPT_SUCCESS) {
        goto error;
    }

    res = ecrypt_auth_sym_plain_encrypt(hdr.alg,
                                        derived_key,
                                        derived_key_length,
                                        hdr.iv,
                                        hdr.iv_length,
                                        user_context,
                                        user_context_length,
                                        message,
                                        message_length,
                                        encrypted_message,
                                        encrypted_message_length,
                                        &auth_tag[0],
                                        &hdr.auth_tag_length);
    if (res != ECRYPT_SUCCESS) {
        goto error;
    }

    /* In valid Secure Cells auth token length always fits into uint32_t. */
    auth_token_real_length = (uint32_t)ecrypt_scell_auth_token_key_size(&hdr);

    if (*auth_token_length < auth_token_real_length) {
        *auth_token_length = auth_token_real_length;
        res = ECRYPT_BUFFER_TOO_SMALL;
        goto error;
    }
    res = ecrypt_write_scell_auth_token_key(&hdr, auth_token, *auth_token_length);
    if (res != ECRYPT_SUCCESS) {
        goto error;
    }
    *auth_token_length = auth_token_real_length;
    *encrypted_message_length = message_length;

error:
    ecconnect_wipe(iv, sizeof(iv));
    ecconnect_wipe(auth_tag, sizeof(auth_tag));
    ecconnect_wipe(derived_key, sizeof(derived_key));

    return res;
}

ecrypt_status_t ecrypt_auth_sym_encrypt_message(const uint8_t* key,
                                                size_t key_length,
                                                const uint8_t* message,
                                                size_t message_length,
                                                const uint8_t* user_context,
                                                size_t user_context_length,
                                                uint8_t* auth_token,
                                                size_t* auth_token_length,
                                                uint8_t* encrypted_message,
                                                size_t* encrypted_message_length)
{
    ECRYPT_CHECK_PARAM(key != NULL && key_length != 0);
    ECRYPT_CHECK_PARAM(message != NULL && message_length != 0);
    if (user_context_length != 0) {
        ECRYPT_CHECK_PARAM(user_context != NULL);
    }
    ECRYPT_CHECK_PARAM(auth_token_length != NULL);
    ECRYPT_CHECK_PARAM(encrypted_message_length != NULL);

    if (!auth_token_length || !encrypted_message || *auth_token_length < default_auth_token_size()
        || *encrypted_message_length < message_length) {
        *auth_token_length = default_auth_token_size();
        *encrypted_message_length = message_length;
        return ECRYPT_BUFFER_TOO_SMALL;
    }

    return ecrypt_auth_sym_encrypt_message_(key,
                                            key_length,
                                            message,
                                            message_length,
                                            user_context,
                                            user_context_length,
                                            auth_token,
                                            auth_token_length,
                                            encrypted_message,
                                            encrypted_message_length);
}

ecrypt_status_t ecrypt_auth_sym_decrypt_message_(const uint8_t* key,
                                                 size_t key_length,
                                                 const uint8_t* user_context,
                                                 size_t user_context_length,
                                                 const uint8_t* auth_token,
                                                 size_t auth_token_length,
                                                 const uint8_t* encrypted_message,
                                                 const size_t encrypted_message_length,
                                                 uint8_t* message,
                                                 size_t* message_length)
{
    ecrypt_status_t res = ECRYPT_FAIL;
    struct ecrypt_scell_auth_token_key hdr;
    /* Use maximum possible length, not the default one */
    uint8_t kdf_context[ECRYPT_AUTH_SYM_MAX_KDF_CONTEXT_LENGTH] = {0};
    uint8_t derived_key[ECRYPT_AUTH_SYM_MAX_KEY_LENGTH / 8] = {0};
    size_t kdf_context_length = sizeof(kdf_context);
    size_t derived_key_length = sizeof(derived_key);

    memset(&hdr, 0, sizeof(hdr));
    res = ecrypt_read_scell_auth_token_key(auth_token, auth_token_length, &hdr);
    if (res != ECRYPT_SUCCESS) {
        return res;
    }

    /* Check that message header is consistent with our expectations */
    if (hdr.message_length != encrypted_message_length) {
        return ECRYPT_FAIL;
    }
    if (!ecconnect_alg_reserved_bits_valid(hdr.alg)) {
        return ECRYPT_FAIL;
    }

    res = ecrypt_auth_sym_kdf_context(hdr.message_length, kdf_context, &kdf_context_length);
    if (res != ECRYPT_SUCCESS) {
        goto error;
    }
    res = ecrypt_auth_sym_derive_encryption_key(hdr.alg,
                                                key,
                                                key_length,
                                                kdf_context,
                                                kdf_context_length,
                                                user_context,
                                                user_context_length,
                                                derived_key,
                                                &derived_key_length);
    if (res != ECRYPT_SUCCESS) {
        goto error;
    }

    res = ecrypt_auth_sym_plain_decrypt(hdr.alg,
                                        derived_key,
                                        derived_key_length,
                                        hdr.iv,
                                        hdr.iv_length,
                                        user_context,
                                        user_context_length,
                                        encrypted_message,
                                        encrypted_message_length,
                                        message,
                                        message_length,
                                        hdr.auth_tag,
                                        hdr.auth_tag_length);
    /*
     * Ecrypt 0.9.6 used slightly different KDF. If decryption fails,
     * maybe it was encrypted with that incorrect key. Try it out.
     */
#ifdef SCELL_COMPAT
    if (res != ECRYPT_SUCCESS && res != ECRYPT_BUFFER_TOO_SMALL) {
        kdf_context_length = sizeof(kdf_context);
        res = ecrypt_auth_sym_kdf_context_compat(hdr.message_length, kdf_context, &kdf_context_length);
        if (res != ECRYPT_SUCCESS) {
            goto error;
        }
        res = ecrypt_auth_sym_derive_encryption_key(hdr.alg,
                                                    key,
                                                    key_length,
                                                    kdf_context,
                                                    kdf_context_length,
                                                    user_context,
                                                    user_context_length,
                                                    derived_key,
                                                    &derived_key_length);
        if (res != ECRYPT_SUCCESS) {
            goto error;
        }
        res = ecrypt_auth_sym_plain_decrypt(hdr.alg,
                                            derived_key,
                                            derived_key_length,
                                            hdr.iv,
                                            hdr.iv_length,
                                            user_context,
                                            user_context_length,
                                            encrypted_message,
                                            encrypted_message_length,
                                            message,
                                            message_length,
                                            hdr.auth_tag,
                                            hdr.auth_tag_length);
    }
#endif

    /* Sanity check of resulting message length */
    if (*message_length != encrypted_message_length) {
        res = ECRYPT_FAIL;
        goto error;
    }

error:
    ecconnect_wipe(derived_key, sizeof(derived_key));

    return res;
}

ecrypt_status_t ecrypt_auth_sym_decrypt_message(const uint8_t* key,
                                                size_t key_length,
                                                const uint8_t* user_context,
                                                size_t user_context_length,
                                                const uint8_t* auth_token,
                                                size_t auth_token_length,
                                                const uint8_t* encrypted_message,
                                                size_t encrypted_message_length,
                                                uint8_t* message,
                                                size_t* message_length)
{
    ecrypt_status_t res = ECRYPT_FAIL;
    uint32_t expected_message_length = 0;

    ECRYPT_CHECK_PARAM(key != NULL && key_length != 0);
    if (user_context_length != 0) {
        ECRYPT_CHECK_PARAM(user_context != NULL);
    }
    ECRYPT_CHECK_PARAM(auth_token != NULL && auth_token_length != 0);
    ECRYPT_CHECK_PARAM(message_length != NULL);

    /* Do a quick guess without parsing the message too deeply here */
    res = ecrypt_scell_auth_token_key_message_size(auth_token, auth_token_length, &expected_message_length);
    if (res != ECRYPT_SUCCESS) {
        return res;
    }
    if (!message || *message_length < expected_message_length) {
        *message_length = expected_message_length;
        return ECRYPT_BUFFER_TOO_SMALL;
    }

    /* encrypted_message may be omitted when only querying plaintext size */
    ECRYPT_CHECK_PARAM(encrypted_message != NULL && encrypted_message_length != 0);

    return ecrypt_auth_sym_decrypt_message_(key,
                                            key_length,
                                            user_context,
                                            user_context_length,
                                            auth_token,
                                            auth_token_length,
                                            encrypted_message,
                                            encrypted_message_length,
                                            message,
                                            message_length);
}

static ecrypt_status_t ecrypt_sym_derive_encryption_key(const uint8_t* key,
                                                        size_t key_length,
                                                        size_t message_length,
                                                        uint8_t* derived_key,
                                                        size_t derived_key_length)
{
    uint8_t kdf_context[sizeof(uint32_t)];
    /*
     * Note that we use only the least significant 32 bits of size_t here.
     * If message_length is longer that 4 GB then the context value gets
     * truncated here. (And on 16-bit platforms it will be zero-padded.)
     */
    stream_write_uint32LE(kdf_context, (uint32_t)message_length);
    return ecrypt_sym_kdf(key,
                          key_length,
                          ECRYPT_SYM_KDF_KEY_LABEL,
                          kdf_context,
                          sizeof(kdf_context),
                          NULL,
                          0,
                          derived_key,
                          derived_key_length);
}

#ifdef SCELL_COMPAT
/*
 * Ecrypt 0.9.6 used 64-bit message length in computations, so here we go.
 */
static ecrypt_status_t ecrypt_sym_derive_encryption_key_compat(const uint8_t* key,
                                                               size_t key_length,
                                                               size_t message_length,
                                                               uint8_t* derived_key,
                                                               size_t derived_key_length)
{
    uint8_t kdf_context[sizeof(uint64_t)];
    stream_write_uint64LE(kdf_context, (uint64_t)message_length);
    return ecrypt_sym_kdf(key,
                          key_length,
                          ECRYPT_SYM_KDF_KEY_LABEL,
                          kdf_context,
                          sizeof(kdf_context),
                          NULL,
                          0,
                          derived_key,
                          derived_key_length);
}
#endif

static ecrypt_status_t ecrypt_sym_derive_encryption_iv(const uint8_t* key,
                                                       size_t key_length,
                                                       const uint8_t* context,
                                                       size_t context_length,
                                                       uint8_t* iv,
                                                       size_t iv_length)
{
    /*
     * Yes, you are reading it correct. We do derive IV with a KDF.
     * Context Imprint mode needs to be completely deterministic.
     * That's why it has lower security than Seal or Token Protect.
     */
    return ecrypt_sym_kdf(key, key_length, ECRYPT_SYM_KDF_IV_LABEL, context, context_length, NULL, 0, iv, iv_length);
}

ecrypt_status_t ecrypt_sym_encrypt_message_u_(const uint8_t* key,
                                              const size_t key_length,
                                              const uint8_t* message,
                                              const size_t message_length,
                                              const uint8_t* context,
                                              const size_t context_length,
                                              uint8_t* encrypted_message,
                                              size_t* encrypted_message_length)
{
    ecrypt_status_t res = ECRYPT_FAIL;
    uint8_t iv[ECRYPT_SYM_IV_LENGTH];

    ECRYPT_CHECK_PARAM(message != NULL && message_length != 0);
    ECRYPT_CHECK_PARAM(context != NULL && context_length != 0);

    if ((*encrypted_message_length) < message_length) {
        (*encrypted_message_length) = message_length;
        return ECRYPT_BUFFER_TOO_SMALL;
    }
    (*encrypted_message_length) = message_length;

    res = ecrypt_sym_derive_encryption_iv(key, key_length, context, context_length, iv, sizeof(iv));
    if (res != ECRYPT_SUCCESS) {
        return res;
    }

    res = ecrypt_sym_plain_encrypt(ECRYPT_SYM_ALG,
                                   key,
                                   key_length,
                                   iv,
                                   sizeof(iv),
                                   message,
                                   message_length,
                                   encrypted_message,
                                   encrypted_message_length);
    ecconnect_wipe(iv, sizeof(iv));
    return res;
}

ecrypt_status_t ecrypt_sym_encrypt_message_u(const uint8_t* key,
                                             const size_t key_length,
                                             const uint8_t* context,
                                             const size_t context_length,
                                             const uint8_t* message,
                                             const size_t message_length,
                                             uint8_t* encrypted_message,
                                             size_t* encrypted_message_length)
{
    ecrypt_status_t res = ECRYPT_FAIL;
    uint8_t derived_key[ECRYPT_SYM_KEY_LENGTH / 8];

    res = ecrypt_sym_derive_encryption_key(key, key_length, message_length, derived_key, sizeof(derived_key));
    if (res != ECRYPT_SUCCESS) {
        return res;
    }

    res = ecrypt_sym_encrypt_message_u_(derived_key,
                                        sizeof(derived_key),
                                        message,
                                        message_length,
                                        context,
                                        context_length,
                                        encrypted_message,
                                        encrypted_message_length);

    ecconnect_wipe(derived_key, sizeof(derived_key));

    return res;
}

ecrypt_status_t ecrypt_sym_decrypt_message_u_(const uint8_t* key,
                                              const size_t key_length,
                                              const uint8_t* context,
                                              const size_t context_length,
                                              const uint8_t* encrypted_message,
                                              const size_t encrypted_message_length,
                                              uint8_t* message,
                                              size_t* message_length)
{
    ecrypt_status_t res = ECRYPT_FAIL;
    uint8_t iv[ECRYPT_SYM_IV_LENGTH];

    ECRYPT_CHECK_PARAM(encrypted_message != NULL && encrypted_message_length != 0);
    ECRYPT_CHECK_PARAM(context != NULL && context_length != 0);

    if ((*message_length) < encrypted_message_length) {
        (*message_length) = encrypted_message_length;
        return ECRYPT_BUFFER_TOO_SMALL;
    }
    (*message_length) = encrypted_message_length;

    res = ecrypt_sym_derive_encryption_iv(key, key_length, context, context_length, iv, sizeof(iv));
    if (res != ECRYPT_SUCCESS) {
        return res;
    }

    res = ecrypt_sym_plain_decrypt(ECRYPT_SYM_ALG,
                                   key,
                                   key_length,
                                   iv,
                                   sizeof(iv),
                                   encrypted_message,
                                   encrypted_message_length,
                                   message,
                                   message_length);
    ecconnect_wipe(iv, sizeof(iv));
    return res;
}

ecrypt_status_t ecrypt_sym_decrypt_message_u(const uint8_t* key,
                                             const size_t key_length,
                                             const uint8_t* context,
                                             const size_t context_length,
                                             const uint8_t* encrypted_message,
                                             const size_t encrypted_message_length,
                                             uint8_t* message,
                                             size_t* message_length)
{
    ecrypt_status_t res = ECRYPT_FAIL;
    uint8_t derived_key[ECRYPT_SYM_KEY_LENGTH / 8];

    res = ecrypt_sym_derive_encryption_key(key,
                                           key_length,
                                           encrypted_message_length,
                                           derived_key,
                                           sizeof(derived_key));
    if (res != ECRYPT_SUCCESS) {
        return res;
    }

    res = ecrypt_sym_decrypt_message_u_(derived_key,
                                        sizeof(derived_key),
                                        context,
                                        context_length,
                                        encrypted_message,
                                        encrypted_message_length,
                                        message,
                                        message_length);
    /*
     * Ecrypt 0.9.6 used slightly different KDF. If decryption fails,
     * maybe it was encrypted with that incorrect key. Try it out.
     */
#ifdef SCELL_COMPAT
    if (res != ECRYPT_SUCCESS && res != ECRYPT_BUFFER_TOO_SMALL) {
        res = ecrypt_sym_derive_encryption_key_compat(key,
                                                      key_length,
                                                      encrypted_message_length,
                                                      derived_key,
                                                      sizeof(derived_key));
        if (res != ECRYPT_SUCCESS) {
            goto error;
        }
        res = ecrypt_sym_decrypt_message_u_(derived_key,
                                            sizeof(derived_key),
                                            context,
                                            context_length,
                                            encrypted_message,
                                            encrypted_message_length,
                                            message,
                                            message_length);
    }

error:
#endif
    ecconnect_wipe(derived_key, sizeof(derived_key));

    return res;
}
