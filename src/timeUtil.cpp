#include "timeUtil.h"
#include <chrono>
#include <cstdint>
#include <ctime>

namespace {

constexpr int BASE_YEAR = 1900;
constexpr int MJD_OFFSET = 15078;
constexpr int MJD_OFFSET_2 = 14956;
constexpr uint64_t UNKNOWN_START_TIME = UINT64_C(0xffffffffff);

inline int convertFromBcd(uint64_t value) {
    return ((value >> 4) & 0xf) * 10 + (value & 0xf);
}

}

void EITDecodeMjd(int i_mjd, int* p_y, int* p_m, int* p_d) {
    const int yp = static_cast<int>((static_cast<double>(i_mjd) - MJD_OFFSET) / 365.25);
    const int mp = static_cast<int>((static_cast<double>(i_mjd) - MJD_OFFSET_2 - static_cast<int>(yp * 365.25)) / 30.6001);
    const int c = (mp == 14 || mp == 15) ? 1 : 0;

    *p_y = BASE_YEAR + yp + c;
    *p_m = mp - 1 - c * 12;
    *p_d = i_mjd - MJD_OFFSET_2 - static_cast<int>(yp * 365.25) - static_cast<int>(mp * 30.6001);
}

struct tm EITConvertStartTime(uint64_t i_date) {
    const int i_mjd = static_cast<int>(i_date >> 24);
    struct tm tm = {};

    tm.tm_hour = convertFromBcd(i_date >> 16);
    tm.tm_min = convertFromBcd(i_date >> 8);
    tm.tm_sec = convertFromBcd(i_date);

    // if all 40 bits are 1, the start is unknown
    if (i_date == UNKNOWN_START_TIME) {
        return {};
    }

    EITDecodeMjd(i_mjd, &tm.tm_year, &tm.tm_mon, &tm.tm_mday);
    tm.tm_year -= BASE_YEAR;
    tm.tm_mon--;
    tm.tm_isdst = 0;

    return tm;
}

bool EITConvertStartTimeToUnixTime(uint64_t i_date, uint64_t* p_unix_time) {
    if (i_date == UNKNOWN_START_TIME || p_unix_time == nullptr) {
        return false;
    }

    const struct tm startTime = EITConvertStartTime(i_date);
    const std::chrono::year_month_day date{
        std::chrono::year{startTime.tm_year + BASE_YEAR},
        std::chrono::month{static_cast<unsigned>(startTime.tm_mon + 1)},
        std::chrono::day{static_cast<unsigned>(startTime.tm_mday)}
    };
    if (!date.ok() || startTime.tm_hour < 0 || startTime.tm_hour > 23 ||
        startTime.tm_min < 0 || startTime.tm_min > 59 ||
        startTime.tm_sec < 0 || startTime.tm_sec > 59) {
        return false;
    }

    constexpr std::chrono::hours JST_OFFSET{9};
    const auto utcTime = std::chrono::sys_days{date} +
        std::chrono::hours{startTime.tm_hour} +
        std::chrono::minutes{startTime.tm_min} +
        std::chrono::seconds{startTime.tm_sec} - JST_OFFSET;
    const auto unixTime = std::chrono::duration_cast<std::chrono::seconds>(
        utcTime.time_since_epoch()).count();
    if (unixTime < 0) {
        return false;
    }

    *p_unix_time = static_cast<uint64_t>(unixTime);
    return true;
}

int EITConvertDuration(uint32_t i_duration) {
    return convertFromBcd(i_duration >> 16) * 3600 +
        convertFromBcd(i_duration >> 8) * 60 +
        convertFromBcd(i_duration);
}