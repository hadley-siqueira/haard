#ifndef HAARD_JSON_VALUE_H
#define HAARD_JSON_VALUE_H

#include <string>
#include <vector>

namespace haard {
    enum JsonValueKind {
        JSON_STRING,
        JSON_DOUBLE,
        JSON_ARRAY,
        JSON_OBJECT,
        JSON_BOOL,
        JSON_INT,
        JSON_NULL
    };

    class Json;
    class JsonValue {
    public:
        JsonValue();
        ~JsonValue();

    public:
        void set_kind(JsonValueKind kind);

        void set_value(bool boolean);
        void set_value(int number);
        void set_value(double number);
        void set_value(std::string str);
        void set_value(Json* obj);

        void add(bool boolean);
        void add(int number);
        void add(double number);
        void add(std::string str);
        void add(Json* json);
        void add(JsonValue* new_value);

        std::string to_str();
        JsonValue* clone();

    private:
        JsonValueKind kind;
        union {
            bool boolean;
            int number_as_int;
            double number_as_double;
            std::string* str;
            std::vector<JsonValue*>* array;
            Json* object;
        } value;
    };
}

#endif
