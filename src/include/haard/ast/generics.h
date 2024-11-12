#ifndef HAARD_AST_GENERICS_H
#define HAARD_AST_GENERICS_H

#include <vector>

#include "haard/ast/expressions/expression.h"
#include "haard/ast/types/type.h"

namespace haard {
    class Generics : public Expression {
    public:
        Generics();
        ~Generics();

    public:
        void add_type(Type* type);

        const std::vector<Type*>& get_types() const;
        void set_types(const std::vector<Type*>& types);

    private:
        std::vector<Type*> types;
    };
}

#endif
