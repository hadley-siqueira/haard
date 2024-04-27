#ifndef HAARD_IR_BINOP_H
#define HAARD_IR_BINOP_H

#include "ir/ir.h"
#include "ir/ir_value.h"

namespace haard {
    class IRBinop : public IR {
    private:
        IRValue* dest;
        IRValue* src1;
        IRValue* src2;
    };
}

#endif
