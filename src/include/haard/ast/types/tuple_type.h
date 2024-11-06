#ifndef HAARD_AST_TUPLE_TYPE_H
#define HAARD_AST_TUPLE_TYPE_H

#include <vector>

#include "haard/ast/types/type.h"

namespace haard {
    class TupleType : public Type {
    public:
        TupleType();
        TupleType(std::vector<Type*>& types);
        ~TupleType();

    public:
        const std::vector<Type*>& get_types() const;
        void set_types(const std::vector<Type*>& types);

        virtual std::string to_json();
        virtual std::string to_str();

    private:
        std::vector<Type*> types;
    };
}

#endif
