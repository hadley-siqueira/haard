#include <sstream>

#include "json/json.h"

using namespace haard;

Json::~Json() {
    for (auto p : pairs) {
        delete p.second;
    }
}

void Json::set(std::string key, int number) {
    JsonValue* value = new JsonValue();

    value->set_value(number);
    set(key, value);
}

void Json::set(std::string key, std::string str) {
    JsonValue* value = new JsonValue();

    value->set_value(str);
    set(key, value);
}

void Json::set(std::string key, JsonValue* value) {
    if (pairs.count(key) > 0) {
        delete pairs[key];
    }

    pairs[key] = value;
}

void Json::add(std::string key, int number) {
    JsonValue* value;

    if (pairs.count(key) == 0) {
        pairs[key] = new JsonValue();
    }

    value = pairs[key];
    value->add(number);
}

void Json::add(std::string key, std::string str) {
    JsonValue* value;

    if (pairs.count(key) == 0) {
        pairs[key] = new JsonValue();
    }

    value = pairs[key];
    value->add(str);
}

void Json::add(std::string key, Json* json) {
    JsonValue* value;

    if (pairs.count(key) == 0) {
        pairs[key] = new JsonValue();
    }

    value = pairs[key];
    value->add(json);
}

void Json::add(std::string key, JsonValue* value) {

}

std::string Json::to_str() {
    std::stringstream ss;
    int i = pairs.size();

    ss << "{";

    for (auto k : pairs) {
        ss << '"' << k.first << "\": ";
        ss << k.second->to_str();

        if (i > 1) {
            ss << ", ";
        }

        --i;
    }

    ss << "}";

    return ss.str();
}

Json* Json::clone() {
    Json* other = new Json();

    for (auto p : pairs) {
        other->set(p.first, p.second->clone());
    }

    return other;
}
