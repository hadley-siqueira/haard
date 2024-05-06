#ifndef HAARD_NAMED_TYPE_H
#define HAARD_NAMED_TYPE_H

#include "ast/type.h"
#include "ast/identifier.h"

namespace haard {
    class NamedType : public Type {
    public:
        NamedType(Identifier* id);
        ~NamedType();

        Identifier* get_identifier() const;
        void set_identifier(Identifier* newIdentifier);

        std::string to_cpp();

    private:
        Identifier* identifier;
    };
}

#endif
