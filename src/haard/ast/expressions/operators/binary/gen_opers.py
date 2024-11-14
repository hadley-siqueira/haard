import re

header = '#include "haard/ast/expressions/operators/binary/OPER_H.h"'
template = """#include "haard/ast/expressions/operators/binary/OPER_H.h"

using namespace haard;

Oper::Oper() {
    set_kind(AST_MOPER);
}

Oper::Oper(Token& token, Expression* left, Expression* right) {
    set_kind(AST_MOPER);
    set_token(token);
    set_left(left);
    set_left(right);
}"""

opers = [
    'assignment', 
    'equal', 'not_equal', 
    'less_than', 'greater_than', 'less_than_or_equal', 'greater_than_or_equal',
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
    content = re.sub("OPER_H", oper, template)
    content = re.sub("Oper", get_name(oper), content)
    content = re.sub("AST_MOPER", "AST_" + oper.upper(), content)
    f = open(oper + ".cc", "w")
    f.write(content)
    f.close()
    print(re.sub("OPER_H", oper, header))

