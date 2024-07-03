#include <sstream>
#include <vector>

#include "json/json_value.h"
#include "json/json.h"

using namespace haard;

JsonValue::JsonValue() {
    value.array = nullptr;
}

JsonValue::~JsonValue() {
    switch (kind) {
    case JSON_STRING:
        delete value.str;
        break;

    case JSON_ARRAY:
        if (value.array) {
            for (int i = 0; i < value.array->size(); ++i) {
                delete value.array->at(i);
            }

            delete value.array;
        }

        break;

    case JSON_OBJECT:
        delete value.object;
        break;

    default:
        break;
    }
}

void JsonValue::set_kind(JsonValueKind kind) {
    this->kind = kind;
}

void JsonValue::set_value(bool boolean) {
    set_kind(JSON_BOOL);
    value.boolean = boolean;
}

void JsonValue::set_value(int number) {
    set_kind(JSON_INT);
    value.number_as_int = number;
}

void JsonValue::set_value(double number) {
    set_kind(JSON_DOUBLE);
    value.number_as_double = number;
}

void JsonValue::set_value(std::string str) {
    set_kind(JSON_STRING);
    value.str = new std::string(str);
}

void JsonValue::set_value(Json* obj) {
    set_kind(JSON_OBJECT);
    value.object = obj;
}

void JsonValue::add(bool boolean) {

}

void JsonValue::add(int number) {
    JsonValue* new_value = new JsonValue();

    new_value->set_value(number);
    set_kind(JSON_ARRAY);

    if (this->value.array == nullptr) {
        this->value.array = new std::vector<JsonValue*>();
    }

    value.array->push_back(new_value);
}

void JsonValue::add(std::string str) {
    JsonValue* new_value = new JsonValue();

    new_value->set_value(str);
    add(new_value);
}

void JsonValue::add(Json* json) {
    JsonValue* new_value = new JsonValue();

    new_value->set_value(json);
    add(new_value);
}

void JsonValue::add(JsonValue* new_value) {
    set_kind(JSON_ARRAY);

    if (this->value.array == nullptr) {
        this->value.array = new std::vector<JsonValue*>();
    }

    value.array->push_back(new_value);
}

std::string JsonValue::to_str() {
    std::stringstream ss;
    std::vector<JsonValue*>* array;
    int i;

    switch (kind) {
    case JSON_NULL:
        ss << "null";
        break;

    case JSON_BOOL:
        if (value.boolean) {
            ss << "true";
        } else {
            ss << "false";
        }
        break;

    case JSON_INT:
        ss << value.number_as_int;
        break;

    case JSON_DOUBLE:
        ss << value.number_as_double;
        break;

    case JSON_STRING:
        ss << '"' << *((std::string*) value.str) << '"';
        break;

    case JSON_ARRAY:
        ss << "[";
        array = (std::vector<JsonValue*>*) value.array;

        if (array && array->size() > 0) {
            for (i = 0; i < array->size() - 1; ++i) {
                ss << array->at(i)->to_str();
                ss << ", ";
            }

            ss << array->at(i)->to_str();
        }

        ss << "]";
        break;

    case JSON_OBJECT:
        ss << ((Json*) value.object)->to_str();
        break;
    }

    return ss.str();
}

JsonValue* JsonValue::clone() {
    int i;
    JsonValue* other = new JsonValue();

    other->set_kind(kind);

    switch (kind) {
    case JSON_NULL:
        break;

    case JSON_BOOL:
        other->set_value(value.boolean);
        break;

    case JSON_INT:
        other->set_value(value.number_as_int);
        break;

    case JSON_DOUBLE:
        other->set_value(value.number_as_double);
        break;

    case JSON_STRING:
        other->set_value(*value.str);
        break;

    case JSON_ARRAY:
        for (i = 0; i < value.array->size(); ++i) {
            other->add(value.array->at(i)->clone());
        }

        break;

    case JSON_OBJECT:
        other->set_value(value.object->clone());
        break;
    }

    return other;
}
