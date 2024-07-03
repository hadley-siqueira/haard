#include <sstream>

#include "json/json.h"
#include "string_pool/string_pool.h"

using namespace haard;

Json::~Json() {
    for (auto p : pairs) {
        delete p.second;
    }
}

void Json::set(std::string key, bool boolean) {
    JsonValue* value = new JsonValue();

    value->set_value(boolean);
    set(key, value);
}

void Json::set(std::string key, int number) {
    JsonValue* value = new JsonValue();

    value->set_value(number);
    set(key, value);
}

void Json::set(std::string key, double number) {
    JsonValue* value = new JsonValue();

    value->set_value(number);
    set(key, value);
}

void Json::set(std::string key, std::string str) {
    JsonValue* value = new JsonValue();

    value->set_value(str);
    set(key, value);
}

void Json::set(std::string key, const char*  str) {
    JsonValue* value = new JsonValue();

    value->set_value(std::string(str));
    set(key, value);
}

void Json::set(std::string key, Json* json) {
    JsonValue* value = new JsonValue();

    value->set_value(json);
    set(key, value);
}

void Json::set(std::string key, JsonValue* value) {
    if (has(key)) {
        remove_key(key);
    }

    set_key_value(key, value);
}

void Json::add(std::string key, int number) {
    JsonValue* value;

    if (!has(key)) {
        set_key_value(key, new JsonValue());
    }

    value = get(key);
    value->add(number);
}

void Json::add(std::string key, std::string str) {
    JsonValue* value;

    if (!has(key)) {
        set_key_value(key, new JsonValue());
    }

    value = get(key);
    value->add(str);
}

void Json::add(std::string key, Json* json) {
    JsonValue* value;

    if (!has(key)) {
        set_key_value(key, new JsonValue());
    }

    value = get(key);
    value->add(json);
}

void Json::add(std::string key, JsonValue* value) {

}


JsonValue* Json::get(std::string key) {
    return pairs[StringPool::get(key)];
    //return pairs[key];
}

bool Json::has(std::string key) {
    return pairs.count(StringPool::get(key)) > 0;
    //return pairs.count(key) > 0;
}

void Json::remove_key(std::string key) {
    delete pairs[StringPool::get(key)];
    pairs.erase(StringPool::get(key));

    //delete pairs[key];
    //pairs.erase(key);
}

void Json::set_key_value(std::string& key, JsonValue* value) {
    pairs[StringPool::get(key)] = value;
    //pairs[key] = value;
}

/*
JsonValue* Json::get(std::string key) {
    for (auto p : pairs) {
        if (p.first == key) {
            return p.second;
        }
    }

    return nullptr;
}

bool Json::has(std::string key) {
    for (auto p : pairs) {
        if (p.first == key) {
            return true;
        }
    }

    return false;
}

void Json::remove_key(std::string key) {
    for (auto p : pairs) {
        if (p.first == key) {
            delete p.second;
        }
    }
}

void Json::set_key_value(std::string& key, JsonValue* value) {
    for (auto p : pairs) {
        if (p.first == key) {
            delete p.second;
            p.second = value;
            return;
        }
    }

    pairs.push_back(make_pair(key, value));
}
*/
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
