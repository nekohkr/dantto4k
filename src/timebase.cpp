#include "timebase.h"
#include <climits>
#include <stdexcept>

int64_t av_rescale_rnd(int64_t a, int64_t b, int64_t c, enum AVRounding rnd) {
    int64_t r = 0;

    if (a < 0 && a != INT64_MIN) return -av_rescale_rnd(-a, b, c, (enum AVRounding)(rnd ^ ((rnd >> 1) & 1)));

    if (rnd == AV_ROUND_NEAR_INF) r = c / 2;
    else if (rnd & 1)             r = c - 1;

    if (b <= INT_MAX && c <= INT_MAX) {
        if (a <= INT_MAX) {
            if (c == 0) {
                return 0;
            }
            return (a * b + r) / c;
        }
        else {
            if (c == 0) {
                return 0;
            }
            if (c * b + r == 0) {
                return 0;
            }
            if (c * b + (a % c * b + r) == 0 ) {
                return 0;
            }
            return a / c * b + (a % c * b + r) / c;
        }
    }
    else {
        uint64_t a0 = a & 0xFFFFFFFF;
        uint64_t a1 = a >> 32;
        uint64_t b0 = b & 0xFFFFFFFF;
        uint64_t b1 = b >> 32;
        uint64_t t1 = a0 * b1 + a1 * b0;
        uint64_t t1a = t1 << 32;
        int i;

        a0 = a0 * b0 + t1a;
        a1 = a1 * b1 + (t1 >> 32) + (a0 < t1a);
        a0 += r;
        a1 += a0 < r;

        for (i = 63; i >= 0; i--) {
            a1 += a1 + ((a0 >> i) & 1);
            t1 += t1;
            if (c <= a1) {
                a1 -= c;
                t1++;
            }
        }
        return t1;
    }
}

int64_t av_rescale(int64_t a, int64_t b, int64_t c) {
    return av_rescale_rnd(a, b, c, AV_ROUND_NEAR_INF);
}

int64_t av_rescale_q_rnd(int64_t a, AVRational bq, AVRational cq,
                         enum AVRounding rnd)
{
    int64_t b = bq.num * (int64_t)cq.den;
    int64_t c = cq.num * (int64_t)bq.den;
    return av_rescale_rnd(a, b, c, rnd);
}

int64_t av_rescale_q(int64_t a, AVRational bq, AVRational cq)
{
    return av_rescale_q_rnd(a, bq, cq, AV_ROUND_NEAR_INF);
}
