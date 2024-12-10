#include <sstream>

#include "haard/ast/statements/elif_statement.h"

using namespace haard;

ElifStatement::ElifStatement() {
    set_kind(AST_ELIF);
}
