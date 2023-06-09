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

#ifndef ECCONNECT_RSA_KEY_PAIR_GEN_H
#define ECCONNECT_RSA_KEY_PAIR_GEN_H

#include <ecconnect/ecconnect_api.h>
#include <ecconnect/ecconnect_error.h>

typedef struct ecconnect_rsa_key_pair_gen_type ecconnect_rsa_key_pair_gen_t;

ECCONNECT_API
ecconnect_rsa_key_pair_gen_t* ecconnect_rsa_key_pair_gen_create(unsigned key_length);

ECCONNECT_API
ecconnect_status_t ecconnect_rsa_key_pair_gen_init(ecconnect_rsa_key_pair_gen_t* ctx, unsigned key_length);

ECCONNECT_API
ecconnect_status_t ecconnect_rsa_key_pair_gen_destroy(ecconnect_rsa_key_pair_gen_t* ctx);

ECCONNECT_API
ecconnect_status_t ecconnect_rsa_key_pair_gen_export_key(ecconnect_rsa_key_pair_gen_t* ctx,
                                                 void* key,
                                                 size_t* key_length,
                                                 bool isprivate);

#endif /* ECCONNECT_RSA_KEY_PAIR_GEN_H */
