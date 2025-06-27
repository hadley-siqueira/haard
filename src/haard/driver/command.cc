#include <haard/driver/command.h>

using namespace haard;

void Command::set_flag(const std::string& flag) {
    this->flag = flag;
}

const std::string& Command::get_flag() {
    return flag;
}

void Command::set_description(const std::string& description) {
    this->description = description;
}

const std::string& Command::get_description() {
    return description;
}

