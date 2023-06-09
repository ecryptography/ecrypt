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

#ifndef ECCONNECT_CRC32_H
#define ECCONNECT_CRC32_H

#include <stddef.h>
#include <stdint.h>

typedef uint32_t ecconnect_crc32_t;

uint32_t ecconnect_crc32c(const uint8_t* buffer, size_t length);

ecconnect_crc32_t ecconnect_crc32_create(void);
void ecconnect_crc32_update(ecconnect_crc32_t* crc, const void* buf, size_t len);
uint32_t ecconnect_crc32_final(ecconnect_crc32_t* crc);

uint32_t ecconnect_crc32(const void* buf, size_t len);

#endif /* ECCONNECT_CRC32_H */
