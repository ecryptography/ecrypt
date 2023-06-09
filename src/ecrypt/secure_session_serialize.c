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

#include <string.h>

#include "ecconnect/ecconnect_container.h"

#include "ecrypt/secure_session.h"
#include "ecrypt/secure_session_t.h"
#include "ecrypt/secure_session_utils.h"
#include "ecrypt/ecrypt_portable_endian.h"

#define ECRYPT_SESSION_CONTEXT_TAG "TSSC"

/*
 * Data layout of serialized Secure Session state looks like this:
 *
 *     0        1        2        3        4        5        6        7
 * +--------+--------+--------+--------+--------+--------+--------+--------+
 * |   'T'  |   'S'  |   'S'  |   'C'  |               size                | ecconnect Container header
 * +--------+--------+--------+--------+--------+--------+--------+--------+
 * |                CRC                |
 * +--------+--------+--------+--------+
 *
 * +--------+--------+--------+--------+--------+--------+--------+--------+
 * |             session ID            |             is_client             | Secure Session context
 * +--------+--------+--------+--------+--------+--------+--------+--------+
 * |                       Secure Session master key                       |
 * + - - - -+ - - - -+ - - - -+ - - - -+ - - - -+ - - - -+ - - - -+ - - - -+
 * |                                                                       |
 * + - - - -+ - - - -+ - - - -+ - - - -+ - - - -+ - - - -+ - - - -+ - - - -+
 * |                                                                       |
 * + - - - -+ - - - -+ - - - -+ - - - -+ - - - -+ - - - -+ - - - -+ - - - -+
 * |                                                                       |
 * +--------+--------+--------+--------+--------+--------+--------+--------+
 * |             out seqnum            |             in seqnum             |
 * +--------+--------+--------+--------+--------+--------+--------+--------+
 *
 * All values are unsigned and encoded as big-endian.
 */

#define SESSION_CTX_SERIALIZED_SIZE \
    (2 * sizeof(uint32_t) + SESSION_MASTER_KEY_LENGTH + 2 * sizeof(uint32_t))

ecrypt_status_t secure_session_save(const secure_session_t* session_ctx, void* out, size_t* out_length)
{
    ecconnect_container_hdr_t* hdr = out;
    uint32_t* curr;

    if ((!session_ctx) || (!out_length)) {
        return ECRYPT_INVALID_PARAMETER;
    }

    if (session_ctx->state_handler) {
        /* Key agreement is not complete. We cannot save session state at this stage. */
        return ECRYPT_INVALID_PARAMETER;
    }

    /* | session_id | is_client | master_key | out_seq | in_seq | */

    if ((!out) || (*out_length < (sizeof(ecconnect_container_hdr_t) + SESSION_CTX_SERIALIZED_SIZE))) {
        *out_length = (sizeof(ecconnect_container_hdr_t) + SESSION_CTX_SERIALIZED_SIZE);
        return ECRYPT_BUFFER_TOO_SMALL;
    }

    *out_length = (sizeof(ecconnect_container_hdr_t) + SESSION_CTX_SERIALIZED_SIZE);

    ecconnect_container_set_data_size(hdr, SESSION_CTX_SERIALIZED_SIZE);
    memcpy(hdr->tag, ECRYPT_SESSION_CONTEXT_TAG, ECCONNECT_CONTAINER_TAG_LENGTH);

    curr = (uint32_t*)ecconnect_container_data(hdr);

    /* session_id */
    *curr = htobe32(session_ctx->session_id);
    curr++;

    /* is_client */
    *curr = htobe32(session_ctx->is_client);
    curr++;

    /* master_key */
    memcpy(curr, session_ctx->session_master_key, sizeof(session_ctx->session_master_key));
    curr += sizeof(session_ctx->session_master_key) / sizeof(uint32_t);

    /* out_seq */
    *curr = htobe32(session_ctx->out_seq);
    curr++;

    /* in_seq */
    *curr = htobe32(session_ctx->in_seq);

    ecconnect_update_container_checksum(hdr);

    return ECRYPT_SUCCESS;
}

ecrypt_status_t secure_session_load(secure_session_t* session_ctx,
                                    const void* in,
                                    size_t in_length,
                                    const secure_session_user_callbacks_t* user_callbacks)
{
    const ecconnect_container_hdr_t* hdr = in;
    ecconnect_status_t ecconnect_res;
    ecrypt_status_t res;
    const uint32_t* curr;

    if ((!session_ctx) || (!in)) {
        return ECRYPT_INVALID_PARAMETER;
    }

    if (in_length < sizeof(ecconnect_container_hdr_t)) {
        return ECRYPT_INVALID_PARAMETER;
    }

    if (in_length < (sizeof(ecconnect_container_hdr_t) + ecconnect_container_data_size(hdr))) {
        return ECRYPT_INVALID_PARAMETER;
    }

    ecconnect_res = ecconnect_verify_container_checksum(hdr);
    if (ECRYPT_SUCCESS != ecconnect_res) {
        return (ecrypt_status_t)ecconnect_res;
    }

    memset(session_ctx, 0, sizeof(secure_session_t)); //Правильно ли
    curr = (const uint32_t*)ecconnect_container_const_data(hdr);

    session_ctx->session_id = be32toh(*curr);
    curr++;

    session_ctx->is_client = be32toh(*curr);
    curr++;

    memcpy(session_ctx->session_master_key, curr, sizeof(session_ctx->session_master_key));
    curr += sizeof(session_ctx->session_master_key) / sizeof(uint32_t);

    /* We have to derive session keys before extracting sequence numbers, because this function
     * overwrites them */
    res = secure_session_derive_message_keys(session_ctx);
    if (ECRYPT_SUCCESS != res) {
        return res;
    }

    session_ctx->out_seq = be32toh(*curr);
    curr++;

    session_ctx->in_seq = be32toh(*curr);

    session_ctx->user_callbacks = user_callbacks;

    return ECRYPT_SUCCESS;
}
