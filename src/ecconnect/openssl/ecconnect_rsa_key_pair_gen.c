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

#include "ecconnect/ecconnect_rsa_key_pair_gen.h"

#include <string.h>

#include <openssl/bn.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>

#include "ecconnect/openssl/ecconnect_engine.h"
#include "ecconnect/ecconnect_rsa_key.h"

static int rsa_key_length(unsigned size)
{
    switch (size) {
    case RSA_KEY_LENGTH_1024:
        return 1024;
    case RSA_KEY_LENGTH_2048:
        return 2048;
    case RSA_KEY_LENGTH_4096:
        return 4096;
    case RSA_KEY_LENGTH_8192:
        return 8192;
    default:
        return 0;
    }
    return 0;
}

ecconnect_rsa_key_pair_gen_t* ecconnect_rsa_key_pair_gen_create(const unsigned key_length)
{
    ECCONNECT_CHECK_PARAM_(rsa_key_length(key_length) > 0);
    ecconnect_rsa_key_pair_gen_t* ctx = malloc(sizeof(ecconnect_rsa_key_pair_gen_t));
    ECCONNECT_CHECK_MALLOC_(ctx);
    ECCONNECT_IF_FAIL_(ecconnect_rsa_key_pair_gen_init(ctx, key_length) == ECCONNECT_SUCCESS, free(ctx));
    return ctx;
}

static ecconnect_status_t ecconnect_set_default_rsa_pub_exp(EVP_PKEY_CTX* pkey_ctx)
{
    BIGNUM* pub_exp;

    pub_exp = BN_new();
    if (!pub_exp) {
        return ECCONNECT_NO_MEMORY;
    }

    if (BN_set_word(pub_exp, RSA_F4) != 1) {
        BN_free(pub_exp);
        return ECCONNECT_FAIL;
    }

    if (EVP_PKEY_CTX_ctrl(pkey_ctx, -1, -1, EVP_PKEY_CTRL_RSA_KEYGEN_PUBEXP, 0, pub_exp) <= 0) {
        BN_free(pub_exp);
        return ECCONNECT_FAIL;
    }
    /* pub_exp is now owned by pkey_ctx */

    return ECCONNECT_SUCCESS;
}

static ecconnect_status_t ecconnect_set_rsa_key_length(EVP_PKEY_CTX* pkey_ctx, int length)
{
    if (EVP_PKEY_CTX_ctrl(pkey_ctx, -1, -1, EVP_PKEY_CTRL_RSA_KEYGEN_BITS, length, NULL) != 1) {
        return ECCONNECT_FAIL;
    }
    return ECCONNECT_SUCCESS;
}

ecconnect_status_t ecconnect_rsa_key_pair_gen_init(ecconnect_rsa_key_pair_gen_t* ctx, const unsigned key_length)
{
    ecconnect_status_t err = ECCONNECT_FAIL;
    EVP_PKEY* pkey = NULL;

    pkey = EVP_PKEY_new();
    if (!pkey) {
        return ECCONNECT_NO_MEMORY;
    }

    /* Only RSA supports asymmetric encryption */
    if (EVP_PKEY_set_type(pkey, EVP_PKEY_RSA) != 1) {
        goto free_pkey;
    }

    ctx->pkey_ctx = EVP_PKEY_CTX_new(pkey, NULL);
    if (!ctx->pkey_ctx) {
        err = ECCONNECT_NO_MEMORY;
        goto free_pkey;
    }

    if (EVP_PKEY_keygen_init(ctx->pkey_ctx) != 1) {
        goto free_pkey_ctx;
    }

    /*
     * Although it seems that OpenSSL/LibreSSL use 0x10001 as default public exponent,
     * we will set it explicitly just in case.
     */
    err = ecconnect_set_default_rsa_pub_exp(ctx->pkey_ctx);
    if (err != ECCONNECT_SUCCESS) {
        goto free_pkey_ctx;
    }

    /*
     * Override default key length for RSA key. Currently OpenSSL has default
     * key length of 1024. LibreSSL has 2048. We will set length explicitly.
     */
    err = ecconnect_set_rsa_key_length(ctx->pkey_ctx, rsa_key_length(key_length));
    if (err != ECCONNECT_SUCCESS) {
        goto free_pkey_ctx;
    }

    if (EVP_PKEY_keygen(ctx->pkey_ctx, &pkey) != 1) {
        goto free_pkey_ctx;
    }

    EVP_PKEY_free(pkey);
    return ECCONNECT_SUCCESS;

free_pkey_ctx:
    EVP_PKEY_CTX_free(ctx->pkey_ctx);
    ctx->pkey_ctx = NULL;
free_pkey:
    EVP_PKEY_free(pkey);
    return err;
}

ecconnect_status_t ecconnect_rsa_key_pair_gen_cleanup(ecconnect_rsa_key_pair_gen_t* ctx)
{
    ECCONNECT_CHECK_PARAM(ctx);
    if (ctx->pkey_ctx) {
        EVP_PKEY_CTX_free(ctx->pkey_ctx);
        ctx->pkey_ctx = NULL;
    }
    return ECCONNECT_SUCCESS;
}

ecconnect_status_t ecconnect_rsa_key_pair_gen_destroy(ecconnect_rsa_key_pair_gen_t* ctx)
{
    ECCONNECT_CHECK_PARAM(ctx);
    ECCONNECT_CHECK(ecconnect_rsa_key_pair_gen_cleanup(ctx) == ECCONNECT_SUCCESS);
    free(ctx);
    return ECCONNECT_SUCCESS;
}

ecconnect_status_t ecconnect_rsa_key_pair_gen_export_key(ecconnect_rsa_key_pair_gen_t* ctx,
                                                 void* key,
                                                 size_t* key_length,
                                                 bool isprivate)
{
    EVP_PKEY* pkey;
    ECCONNECT_CHECK_PARAM(ctx);
    pkey = EVP_PKEY_CTX_get0_pkey(ctx->pkey_ctx);
    ECCONNECT_CHECK_PARAM(pkey);
    ECCONNECT_CHECK_PARAM(EVP_PKEY_RSA == EVP_PKEY_id(pkey));
    if (isprivate) {
        return ecconnect_engine_specific_to_rsa_priv_key((const ecconnect_engine_specific_rsa_key_t*)pkey,
                                                     (ecconnect_container_hdr_t*)key,
                                                     key_length);
    }

    return ecconnect_engine_specific_to_rsa_pub_key((const ecconnect_engine_specific_rsa_key_t*)pkey,
                                                (ecconnect_container_hdr_t*)key,
                                                key_length);
}
