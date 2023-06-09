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

#include "ge_utils.h"
#include <ecconnect/ecconnect.h>

void clip_random_32(unsigned char *r)
{
    r[0] &= 248;
    r[31] &= 63;
    r[31] |= 64;
}

void generate_random_32(unsigned char *r)
{
	ecconnect_status_t res = ecconnect_rand(r, ED25519_GE_LENGTH);

	if (ECCONNECT_SUCCESS != res)
	{
		/* TODO: How to handle this to keep non-error nature of ed25519 ? */
		exit(1);
	}

	clip_random_32(r);
}
