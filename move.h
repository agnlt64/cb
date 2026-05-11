#pragma once

#include <stdint.h>

#include "square.h"

// 6 bits = from, 6 bits = to, 4 bits = flags, 4 bits = captured
typedef uint32_t move_t;

typedef enum move_flag
{
    FLAG_QUIET = 0,
    FLAG_CAPTURE,
    FLAG_EP,
    FLAG_CASTLE_K,
    FLAG_CASTLE_Q,
    FLAG_PROMO_N,
    FLAG_PROMO_B,
    FLAG_PROMO_R,
    FLAG_PROMO_Q,
} move_flag_t;

#define MOVE_FROM(m) ((m) & 0x3F)
#define MOVE_TO(m) (((m) >> 6) & 0x3F)
#define MOVE_FLAGS(m) (((m) >> 12) & 0xF)
#define MOVE_CAPTURED(m) (((m) >> 16) & 0xF)
#define MOVE_ENCODE(from, to, flags, captured) \
      ((from) | ((to) << 6) | ((flags) << 12) | ((captured) << 16))

void move_print(move_t move);
char* move_to_uci(move_t move);