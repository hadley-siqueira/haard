import re

header = '#include "haard/ast/expressions/operators/unary/OPER_H.h"'
template = """#include "haard/ast/expressions/operators/unary/OPER_H.h"

using namespace haard;

Oper::Oper() {
    set_kind(AST_MOPER);
}

Oper::Oper(Token& token, Expression* expression) {
    set_kind(AST_MOPER);
    set_token(token);
    set_expression(expression);
}"""

opers = [
    'unary_plus', 'unary_minus', 'address_of', 'dereference', 'bitwise_not', 'logical_not',
    'pre_increment', 'pre_decrement', 'pos_increment', 'pos_decrement', 'parenthesis',
    'delete', 'delete_array'
]


def get_name(n):
    names = n.split('_')
    tmp = ""

    for name in names:
        tmp += name.capitalize()

    return tmp

for oper in opers:
    content = re.sub("OPER_H", oper, template)
    content = re.sub("Oper", get_name(oper), content)
    content = re.sub("AST_MOPER", "AST_" + oper.upper(), content)
    f = open(oper + ".cc", "w")
    f.write(content)
    f.close()
    print(re.sub("OPER_H", oper, header))

