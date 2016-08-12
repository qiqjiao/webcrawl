#include "Base64.h"

#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/algorithm/string.hpp>

namespace base {

using namespace boost::archive::iterators;

std::string base64_encode(const std::string &val) {
    using It = base64_from_binary<transform_width<std::string::const_iterator, 6, 8>>;
    auto tmp = std::string(It(std::begin(val)), It(std::end(val)));
    return tmp.append((3 - val.size() % 3) % 3, '=');
}

std::string base64_decode(const std::string &val) {
    using It = transform_width<binary_from_base64<std::string::const_iterator>, 8, 6>;
    std::string r(It(std::begin(val)), It(std::end(val)));
    for (int i = val.size() - 1; i >= 0 && val[i] == '='; --i) {
      r.pop_back();
    }
    return r;
}

} // namespace base
