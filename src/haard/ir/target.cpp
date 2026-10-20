#include <haard/ir/target.h>

using namespace haard;

bool haard::find_target(const std::string& name, Target& target) {
    if (name == "x86_64-linux" || name == "arm64-linux") {
        target.name = name;
        target.big_endian = false;
        target.pointer_size = 8;
        return true;
    }

    if (name == "pret") {
        target.name = name;
        target.big_endian = true;
        target.pointer_size = 8;
        return true;
    }

    return false;
}
