#include <set>
#include "ast/types/type.h"
#include "type_pool/type_pool.h"

using namespace haard;

TypePool type_pool;

Type::Type() {
    type_pool.add_type(this);
}

Type::Type(int kind) {
    set_kind(kind);
}

Type::~Type() {

}

Type::Type(int kind, const Token& token) {
    set_kind(kind);
    set_token(token);
    type_pool.add_type(this);
}

std::string Type::to_cpp() {
    switch (get_kind()) {
    case AST_TYPE_U8:
        return "u8";

    case AST_TYPE_U16:
        return "u16";

    case AST_TYPE_U32:
        return "u32";

    case AST_TYPE_U64:
        return "u64";

    case AST_TYPE_I8:
        return "i8";

    case AST_TYPE_I16:
        return "i16";

    case AST_TYPE_I32:
        return "i32";

    case AST_TYPE_I64:
        return "i64";

    case AST_TYPE_BOOL:
        return "bool";

    case AST_TYPE_CHAR:
        return "char";

    case AST_TYPE_UCHAR:
        return "unsigned char";

    case AST_TYPE_SHORT:
        return "i16";

    case AST_TYPE_USHORT:
        return "u16";

    case AST_TYPE_INT:
        return "i32";

    case AST_TYPE_UINT:
        return "u32";

    case AST_TYPE_LONG:
        return "i64";

    case AST_TYPE_ULONG:
        return "u64";

    case AST_TYPE_VOID:
        return "void";
    }

    return "unknown type";
}

bool Type::equals(Type* other) {
    return get_kind() == other->get_kind();
}
