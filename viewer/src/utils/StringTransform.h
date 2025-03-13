#ifndef UTILS_STRING_TRANSFORM_H_
#define UTILS_STRING_TRANSFORM_H_

#include <QString>
#include <string>

namespace Utils {

auto toQString(double num) -> QString;
auto toQString(int32_t num) -> QString;
auto toQString(int64_t num) -> QString;
auto getMemorySizeUnit(int64_t size) -> QString;
auto getMemorySizeNumString(int64_t size) -> QString;
auto tagCodeToVersion(uint16_t tagCode) -> std::string;

}  // namespace Utils

#endif  // UTILS_STRING_TRANSFORM_H_