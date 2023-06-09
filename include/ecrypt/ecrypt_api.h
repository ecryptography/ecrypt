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

#ifndef ECRYPT_API_H
#define ECRYPT_API_H

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

#if defined(__GNUC__) || defined(__clang__)
#define ECRYPT_API EMSCRIPTEN_KEEPALIVE __attribute__((visibility("default")))
#elif defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
#ifdef ECRYPT_EXPORT
#define ECRYPT_API EMSCRIPTEN_KEEPALIVE __declspec(dllexport)
#else
#define ECRYPT_API EMSCRIPTEN_KEEPALIVE __declspec(dllimport)
#endif
#else
#define ECRYPT_API EMSCRIPTEN_KEEPALIVE
#endif

/*
 * Marks API that needs to be exported for technical reasons, but otherwise
 * is not intended for user consumption.
 */
#define ECRYPT_PRIVATE_API ECRYPT_API

#endif /* ECRYPT_API_H */
