#include <haard/ast/statements.h>

using namespace haard;

void Statements::add_statement(Statement* statement) {
    if (statement) {
        statements.push_back(statement);
        statement->set_parent_node(this);
    }
}
