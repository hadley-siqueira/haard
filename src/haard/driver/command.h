#ifndef HAARD_COMMAND_H
#define HAARD_COMMAND_H

#include <string>

namespace haard {
    class Command {
    public:
        Command();
        ~Command();

    public:
        void set_flag(const std::string& flag);
        const std::string& get_flag();

        void set_description(const std::string& description);
        const std::string& get_description();

    private:
        std::string flag;
        std::string description;
    };
}

#endif
