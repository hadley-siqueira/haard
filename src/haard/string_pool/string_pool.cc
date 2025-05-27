#include <unordered_set>
#include <string>

#include <haard/string_pool/string_pool.h>

using namespace haard;

std::unordered_set<std::string> values;

char* StringPool::get(const char* value) {
    if (values.count(value) == 0) {
        values.insert(value);
    }

    return (char*) (*values.find(value)).c_str();
}
