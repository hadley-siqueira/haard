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

opers = ['assignment', 'dot', 'arrow']

for oper in opers:
    content = re.sub("OPER_H", oper, template)
    content = re.sub("Oper", oper.capitalize(), content)
    content = re.sub("AST_MOPER", "AST_" + oper.upper(), content)
    f = open(oper + ".cc", "w")
    f.write(content)
    f.close()
    print(re.sub("OPER_H", oper, header))

