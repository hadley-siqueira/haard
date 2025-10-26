#include <haard/string_pool/string_pool.h>
#include <unordered_set>

using namespace haard;

std::unordered_set<std::string> values;

const char* StringPool::get(const std::string& value) {
    auto v = values.insert(value);

    return v.first->c_str();
}

