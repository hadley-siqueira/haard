#include <sstream>

#include "haard/ast/statements/compound_statement.h"

using namespace haard;

CompoundStatement::~CompoundStatement() {
    for (auto s : statements) {
        delete s;
    }
}

std::string CompoundStatement::to_json() {
    return "compound statement json";
}

std::string CompoundStatement::to_str() {
    std::stringstream ss;
    bool first = true;

    for (auto s : statements) {
        if (!first) ss << "\n";
        ss << s->to_str();
        first = false;
    }

    return ss.str();
}
