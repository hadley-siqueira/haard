#ifndef HAARD_IR_H
#define HAARD_IR_H

namespace haard {
    enum IRKind {
        IR_UNKNOWN,
        IR_ADD,
        IR_SUB,
    };

    class IR {
    public:
        IR();

    public:
        IRKind get_kind() const;
        void set_kind(IRKind newKind);

    private:
        IRKind kind;
    };
}

#endif
