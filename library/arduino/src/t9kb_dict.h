// T9KB - Developed by acidvegas in C (https://github.com/acidvegas)
// library/arduino/src/t9kb_dict.h - the T9 word list built by library/tools/gen_dict.py
#pragma once

#include <stdint.h>

// The word list takes about 115 KB of flash, more than AVR boards have. Define T9KB_DICT 0 to leave it out anywhere.
#ifndef T9KB_DICT
#if defined(__AVR__)
#define T9KB_DICT 0
#else
#define T9KB_DICT 1
#endif
#endif

#if T9KB_DICT
#ifdef __cplusplus
extern "C" {
#endif
extern const uint16_t t9kb_dict_words;
extern const uint16_t t9kb_dict_blocks;
extern const uint8_t  t9kb_dict_block;
extern const uint32_t t9kb_dict_index[];
extern const uint8_t  t9kb_dict_data[];
#ifdef __cplusplus
}
#endif
#endif
