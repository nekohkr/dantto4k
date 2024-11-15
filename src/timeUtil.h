#pragma once
#include <cstdint>
#include <ctime>

void EITDecodeMjd(int i_mjd, int* p_y, int* p_m, int* p_d);
struct tm EITConvertStartTime(uint64_t i_date);
int EITConvertDuration(uint32_t i_duration);