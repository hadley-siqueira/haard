#ifndef HAARD_IR_VALUE_H
#define HAARD_IR_VALUE_H

namespace haard {
    enum IRValueKind {
        IR_VALUE_TEMP,
    };

    class IRValue {
    public:

        IRValueKind get_kind() const;
        void set_kind(IRValueKind newKind);

    private:
        IRValueKind kind;
    };
}

#endif
