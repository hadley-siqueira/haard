#ifndef HAARD_JSON_H
#define HAARD_JSON_H

#include <map>
#include <string>
#include "json/json_value.h"

namespace haard {
    class Json {
    public:
        ~Json();

    public:
        void set(std::string key, int number);
        void set(std::string key, std::string str);
        void set(std::string key, JsonValue* value);

        void add(std::string key, int number);
        void add(std::string key, std::string str);
        void add(std::string key, Json* json);
        void add(std::string key, JsonValue* value);

        std::string to_str();
        Json* clone();

    private:
        std::map<std::string, JsonValue*> pairs;
    };
}

#endif
