#include <sstream>

#include "haard/ast/types/primitive_type.h"

using namespace haard;

PrimitiveType::PrimitiveType() {

}

PrimitiveType::PrimitiveType(AstKind kind) {
    set_kind(kind);
}

PrimitiveType::PrimitiveType(AstKind kind, Token& token) {
    set_kind(kind);
    set_token(token);
}

const Token& PrimitiveType::get_token() const {
    return token;
}

void PrimitiveType::set_token(const Token& token) {
    this->token = token;
}

std::string PrimitiveType::to_json() {
    std::stringstream ss;

    ss << "{";

    ss << "};";
    return ss.str();
}

std::string PrimitiveType::to_str() {
    switch (kind) {
    case AST_TYPE_BOOL:
        return "bool";

    case AST_TYPE_CHAR:
        return "char";

    case AST_TYPE_SYMBOL:
        return "sym";

    case AST_TYPE_VOID:
        return "void";

    case AST_TYPE_STR:
        return "str";

    case AST_TYPE_I8:
        return "i8";

    case AST_TYPE_I16:
        return "i16";

    case AST_TYPE_I32:
        return "i32";

    case AST_TYPE_I64:
        return "i64";

    case AST_TYPE_U8:
        return "u8";

    case AST_TYPE_U16:
        return "u16";

    case AST_TYPE_U32:
        return "u32";

    case AST_TYPE_U64:
        return "u64";

    case AST_TYPE_F32:
        return "f32";

    case AST_TYPE_F64:
        return "f64";

    default:
        return "unknown primitive type";
    }
}
