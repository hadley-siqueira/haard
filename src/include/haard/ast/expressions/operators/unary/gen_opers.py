import re

header = '#include "haard/ast/expressions/operators/unary/OPER_H.h"'

template = """#ifndef HAARD_AST_HOPER_H
#define HAARD_AST_HOPER_H

#include "haard/ast/expressions/operators/unary/unary_operator.h"

namespace haard {
    class OPer : public UnaryOperator {
    public:
        OPer();
        OPer(Token& token, Expression* expression=nullptr);
    };
}

#endif"""

opers = [
    'unary_plus', 'unary_minus', 'address_of', 'dereference', 'bitwise_not', 'logical_not',
    'pre_increment', 'pre_decrement', 'pos_increment', 'pos_decrement', 'parenthesis'
]

def get_name(n):
    names = n.split('_')
    tmp = ""

    for name in names:
        tmp += name.capitalize()

    return tmp

for oper in opers:
    content = re.sub("HOPER", oper.upper(), template)
    content = re.sub("OPer", get_name(oper), content)
    content = re.sub("AST_MOPER", "AST_" + oper.upper(), content)
    f = open(oper + ".h", "w")
    f.write(content)
    f.close()
    print(re.sub("OPER_H", oper, header))

