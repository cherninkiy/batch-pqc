/*
   This product is distributed under 2-term BSD-license terms

   Copyright (c) 2023, QApp. All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
   ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
   ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include <stdint.h>

/* Hypericum debug profile: fast diagnostics only. */
/* Height of the hypertree. */
#define HYP_H 10
/* Hypertree layers count */
#define HYP_D 2
/* FORS+C tree height */
#define HYP_B 5
/* FORS+C trees count plus one */
#define HYP_K 8

/*
 * checks whether bits in interval [(k - 1) * b; k * b) of digest are all 0
 * this variant is generic and works for any HYP_B/HYP_K combination.
 */
static uint8_t md_suffix_nonzero(const uint8_t* digest)
{
    uint16_t i;
    uint16_t start = (uint16_t)((HYP_K - 1) * HYP_B);
    uint8_t acc = 0;

    for (i = 0; i < HYP_B; ++i) {
        uint16_t bit_index = (uint16_t)(start + i);
        uint16_t byte_index = (uint16_t)(bit_index >> 3);
        uint8_t bit_in_byte = (uint8_t)(7U - (bit_index & 7U));
        acc |= (uint8_t)(digest[byte_index] & (uint8_t)(1U << bit_in_byte));
    }

    return acc;
}
