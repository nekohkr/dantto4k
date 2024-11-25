#pragma once
#include <cstdint>

typedef struct AVRational{
    int num; ///< Numerator
    int den; ///< Denominator
} AVRational;

enum AVRounding {
    AV_ROUND_ZERO = 0,
    AV_ROUND_INF = 1,
    AV_ROUND_DOWN = 2,
    AV_ROUND_UP = 3,
    AV_ROUND_NEAR_INF = 5,
};

int64_t av_rescale_rnd(int64_t a, int64_t b, int64_t c, enum AVRounding rnd);

int64_t av_rescale(int64_t a, int64_t b, int64_t c);

int64_t av_rescale_q_rnd(int64_t a, AVRational bq, AVRational cq, enum AVRounding rnd);

int64_t av_rescale_q(int64_t a, AVRational bq, AVRational cq);