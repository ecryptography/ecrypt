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

#include "ecconnect/ecconnect_sign_rsa.h"

#include <openssl/evp.h>
#include <openssl/rsa.h>

#include "ecconnect/boringssl/ecconnect_engine.h"
#include "ecconnect/boringssl/ecconnect_rsa_common.h"
#include "ecconnect/ecconnect_rsa_key.h"

ecconnect_status_t ecconnect_verify_init_rsa_pss_pkcs8(ecconnect_sign_ctx_t* ctx,
                                               const void* private_key,
                                               const size_t private_key_length,
                                               const void* public_key,
                                               const size_t public_key_length)
{
    ecconnect_status_t err = ECCONNECT_FAIL;
    EVP_PKEY* pkey = NULL;
    EVP_PKEY_CTX* md_pkey_ctx = NULL;

    pkey = EVP_PKEY_new();
    if (!pkey) {
        return ECCONNECT_NO_MEMORY;
    }

    if (!EVP_PKEY_set_type(pkey, EVP_PKEY_RSA)) {
        goto free_pkey;
    }

    ctx->pkey_ctx = EVP_PKEY_CTX_new(pkey, NULL);
    if (!(ctx->pkey_ctx)) {
        err = ECCONNECT_NO_MEMORY;
        goto free_pkey;
    }

    if (private_key && private_key_length != 0) {
        err = ecconnect_rsa_import_key(pkey, private_key, private_key_length);
        if (err != ECCONNECT_SUCCESS) {
            goto free_pkey_ctx;
        }
    }
    if (public_key && public_key_length != 0) {
        err = ecconnect_rsa_import_key(pkey, public_key, public_key_length);
        if (err != ECCONNECT_SUCCESS) {
            goto free_pkey_ctx;
        }
    }

    ctx->md_ctx = EVP_MD_CTX_create();
    if (!(ctx->md_ctx)) {
        err = ECCONNECT_NO_MEMORY;
        goto free_pkey_ctx;
    }

    /* md_pkey_ctx is owned by ctx->md_ctx */
    if (!EVP_DigestVerifyInit(ctx->md_ctx, &md_pkey_ctx, EVP_sha256(), NULL, pkey)) {
        goto free_md_ctx;
    }
    if (!EVP_PKEY_CTX_set_rsa_padding(md_pkey_ctx, RSA_PKCS1_PSS_PADDING)) {
        goto free_md_ctx;
    }
    if (!EVP_PKEY_CTX_set_rsa_pss_saltlen(md_pkey_ctx, -2)) {
        goto free_md_ctx;
    }

    EVP_PKEY_free(pkey);
    return ECCONNECT_SUCCESS;

free_md_ctx:
    EVP_MD_CTX_destroy(ctx->md_ctx);
    ctx->md_ctx = NULL;
free_pkey_ctx:
    EVP_PKEY_CTX_free(ctx->pkey_ctx);
    ctx->pkey_ctx = NULL;
free_pkey:
    EVP_PKEY_free(pkey);
    return err;
}

ecconnect_status_t ecconnect_verify_update_rsa_pss_pkcs8(ecconnect_sign_ctx_t* ctx,
                                                 const void* data,
                                                 const size_t data_length)
{
    if (!EVP_DigestVerifyUpdate(ctx->md_ctx, data, data_length)) {
        return ECCONNECT_FAIL;
    }
    return ECCONNECT_SUCCESS;
}

ecconnect_status_t ecconnect_verify_final_rsa_pss_pkcs8(ecconnect_sign_ctx_t* ctx,
                                                const void* signature,
                                                const size_t signature_length)
{
    if (!ctx) {
        return ECCONNECT_INVALID_PARAMETER;
    }
    EVP_PKEY* pkey = EVP_PKEY_CTX_get0_pkey(ctx->pkey_ctx);
    if (!pkey) {
        return ECCONNECT_INVALID_PARAMETER;
    }
    if (signature_length != (size_t)EVP_PKEY_size(pkey)) {
        return ECCONNECT_FAIL;
    }
    if (!EVP_DigestVerifyFinal(ctx->md_ctx, (unsigned char*)signature, signature_length)) {
        return ECCONNECT_FAIL;
    }
    return ECCONNECT_SUCCESS;
}

ecconnect_status_t ecconnect_verify_cleanup_rsa_pss_pkcs8(ecconnect_sign_ctx_t* ctx)
{
    if (!ctx) {
        return ECCONNECT_INVALID_PARAMETER;
    }
    if (ctx->pkey_ctx) {
        EVP_PKEY_CTX_free(ctx->pkey_ctx);
        ctx->pkey_ctx = NULL;
    }
    if (ctx->md_ctx) {
        EVP_MD_CTX_destroy(ctx->md_ctx);
        ctx->md_ctx = NULL;
    }
    return ECCONNECT_SUCCESS;
}
