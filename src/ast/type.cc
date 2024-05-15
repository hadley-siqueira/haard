#include <set>
#include "ast/type.h"
#include "type_pool/type_pool.h"

using namespace haard;

TypePool type_pool;

Type::Type() {
    type_pool.add_type(this);
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
    case TYPE_U8:
        return "u8";

    case TYPE_U16:
        return "u16";

    case TYPE_U32:
        return "u32";

    case TYPE_U64:
        return "u64";

    case TYPE_I8:
        return "i8";

    case TYPE_I16:
        return "i16";

    case TYPE_I32:
        return "i32";

    case TYPE_I64:
        return "i64";

    case TYPE_BOOL:
        return "bool";

    case TYPE_CHAR:
        return "char";

    case TYPE_UCHAR:
        return "unsigned char";

    case TYPE_SHORT:
        return "i16";

    case TYPE_USHORT:
        return "u16";

    case TYPE_INT:
        return "i32";

    case TYPE_UINT:
        return "u32";

    case TYPE_LONG:
        return "i64";

    case TYPE_ULONG:
        return "u64";

    case TYPE_VOID:
        return "void";
    }

    return "unknown type";
}

bool Type::equals(Type* other) {
    return get_kind() == other->get_kind();
}
