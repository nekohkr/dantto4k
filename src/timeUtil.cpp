#include "timeUtil.h"

void EITDecodeMjd(int i_mjd, int* p_y, int* p_m, int* p_d)
{
    const int yp = (int)(((double)i_mjd - 15078.2) / 365.25);
    const int mp = (int)(((double)i_mjd - 14956.1 - (int)(yp * 365.25)) / 30.6001);
    const int c = (mp == 14 || mp == 15) ? 1 : 0;

    *p_y = 1900 + yp + c * 1;
    *p_m = mp - 1 - c * 12;
    *p_d = i_mjd - 14956 - (int)(yp * 365.25) - (int)(mp * 30.6001);
}

#define CVT_FROM_BCD(v) ((((v) >> 4)&0xf)*10 + ((v)&0xf))
struct tm EITConvertStartTime(uint64_t i_date)
{
    const int i_mjd = static_cast<int>(i_date >> 24);
    struct tm tm;

    tm.tm_hour = CVT_FROM_BCD(i_date >> 16);
    tm.tm_min = CVT_FROM_BCD(i_date >> 8);
    tm.tm_sec = CVT_FROM_BCD(i_date);

    /* if all 40 bits are 1, the start is unknown */
    if (i_date == UINT64_C(0xffffffffff))
        return {};

    EITDecodeMjd(i_mjd, &tm.tm_year, &tm.tm_mon, &tm.tm_mday);
    tm.tm_year -= 1900;
    tm.tm_mon--;
    tm.tm_isdst = 0;

    return tm;
}

int EITConvertDuration(uint32_t i_duration)
{
    return CVT_FROM_BCD(i_duration >> 16) * 3600 +
        CVT_FROM_BCD(i_duration >> 8) * 60 +
        CVT_FROM_BCD(i_duration);
}
