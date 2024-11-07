#include "haard/ast/statements/statement.h"

using namespace haard;

Statement::Statement() {
    set_kind(AST_UNKNOWN);
}

Statement::~Statement() {

}
