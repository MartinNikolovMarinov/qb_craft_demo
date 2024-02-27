#pragma once

#include <string>

namespace core {

std::string genRndStr(int32_t len);
int32_t genRndInt32(int32_t min, int32_t max);
int64_t genRndInt64(int64_t min, int64_t max);

} // namespace core
