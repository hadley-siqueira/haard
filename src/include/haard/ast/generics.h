#ifndef HAARD_AST_GENERICS_H
#define HAARD_AST_GENERICS_H

#include <vector>

#include "haard/ast/ast.h"
#include "haard/ast/types/type.h"

namespace haard {
    class Generics : public Ast {
    public:
        Generics();
        ~Generics();

    public:
        void add_type(Type* type);

        const std::vector<Type*>& get_types() const;
        void set_types(const std::vector<Type*>& types);

        virtual std::string to_json();
        virtual std::string to_str();

    private:
        std::vector<Type*> types;
    };
}

#endif
