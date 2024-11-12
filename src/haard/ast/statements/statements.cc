#include <sstream>

#include "haard/ast/statements/statements.h"

using namespace haard;

Statements::Statements() {
    set_kind(AST_STATEMENTS);
}

Statements::~Statements() {
    for (auto s : statements) {
        delete s;
    }
}

void Statements::add_statement(Statement* statement) {
    if (statement) {
        statements.push_back(statement);
        statement->set_parent(this);
    }
}
/*
std::string Statements::to_json() {
    return "compound statement json";
}

std::string Statements::to_str() {
    std::stringstream ss;
    bool first = true;

    if (statements.size() == 0) {
        return "pass";
    }

    for (auto s : statements) {
        if (!first) ss << "\n";
        ss << s->to_str();
        first = false;
    }

    return ss.str();
}*/
