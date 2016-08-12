#pragma once

#include <string>

namespace base {

std::string base64_encode(const std::string &s);
std::string base64_decode(const std::string &s);

} // namespace base
