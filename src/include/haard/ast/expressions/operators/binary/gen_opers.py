import re

header = '#include "haard/ast/expressions/operators/binary/OPER_H.h"'

template = """#ifndef HAARD_AST_HOPER_H
#define HAARD_AST_HOPER_H

#include "haard/ast/expressions/operators/binary/binary_operator.h"

namespace haard {
    class OPer : public BinaryOperator {
    public:
        OPer();
        OPer(Token& token, Expression* left=nullptr, Expression* right=nullptr);
    };
}

#endif"""

opers = [
    'assignment', 'bitwise_and_assignment', 'bitwise_or_assignment', 'bitwise_xor_assignment',
    'bitwise_not_assignment', 'division_assignment', 'integer_division_assignment', 
    'minus_assignment', 'modulo_assignment', 'plus_assignment', 'times_assignment',
    'shift_left_logical_assignment',
    'shift_right_logical_assignment', 'shift_right_arithmetic_assignment',
    'logical_and', 'logical_or',
    'equal', 'not_equal', 
    'less_than', 'greater_than', 'less_than_or_equal', 'greater_than_or_equal', 'in', 'not_in',
    'inclusive_range', 'exclusive_range', 
    'plus', 'minus', 
    'times', 'division', 'modulo', 'integer_division', 'power', 
    'shift_left_logical', 'shift_right_logical', 'shift_right_arithmetic',
    'bitwise_and', 'bitwise_or', 'bitwise_xor', 
    'index', 'dot', 'arrow'
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

