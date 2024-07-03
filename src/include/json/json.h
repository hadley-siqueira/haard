#ifndef HAARD_JSON_H
#define HAARD_JSON_H

#include <map>
#include <unordered_map>
#include <list>
#include <string>
#include <vector>
#include "json/json_value.h"

namespace haard {
    class Json {
    public:
        ~Json();

    public:
        void set(std::string key, bool boolean);
        void set(std::string key, int number);
        void set(std::string key, double number);
        void set(std::string key, std::string str);
        void set(std::string key, const char* str);
        void set(std::string key, Json* json);
        void set(std::string key, JsonValue* value);

        void add(std::string key, int number);
        void add(std::string key, std::string str);
        void add(std::string key, Json* json);
        void add(std::string key, JsonValue* value);

        JsonValue* get(std::string key);
        bool has(std::string key);
        void remove_key(std::string key);
        void set_key_value(std::string &key, JsonValue* value);

        std::string to_str();
        Json* clone();

    private:
        std::map<const char*, JsonValue*> pairs;
        //std::unordered_map<std::string, JsonValue*> pairs;
        //std::list<std::pair<std::string, JsonValue*>> pairs;
        //std::vector<std::pair<std::string, JsonValue*>> pairs;
    };
}

#endif
