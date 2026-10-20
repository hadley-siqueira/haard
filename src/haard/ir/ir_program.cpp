#include <haard/ir/ir_program.h>

using namespace haard;

IrProgram::IrProgram() {
    IrObject nothing;
    IrSignature none;

    nothing.kind = IR_OBJECT_NONE;
    nothing.function = 0;
    nothing.type.index = 0;
    nothing.zeroed = false;
    nothing.read_only = false;
    none.result.index = 0;

    // index 0 of each is 'none'
    objects.push_back(nothing);
    functions.push_back(IrFunction());
    signatures.push_back(none);
    classes.set_types(&types);
}

IrTypeTable* IrProgram::get_types() {
    return &types;
}

IrClassTable* IrProgram::get_classes() {
    return &classes;
}

u32 IrProgram::signature(const std::vector<IrTypeId>& parameters,
                         IrTypeId result) {
    std::vector<u32> key;

    for (IrTypeId one : parameters) {
        key.push_back(one.index);
    }

    key.push_back(result.index);

    std::map<std::vector<u32>, u32>::iterator found =
        signature_index.find(key);

    if (found != signature_index.end()) {
        return found->second;
    }

    IrSignature made;

    made.parameters = parameters;
    made.result = result;
    signatures.push_back(made);
    signature_index[key] = (u32) signatures.size() - 1;

    return (u32) signatures.size() - 1;
}

const IrSignature& IrProgram::get_signature(u32 index) {
    return signatures[index];
}

IrObjectId IrProgram::add_function(const std::string& name, u32 signature,
                                   IrForm form) {
    IrObject object;
    IrFunction body;
    IrObjectId id = {(u32) objects.size()};

    object.kind = IR_OBJECT_FUNCTION;
    object.name = name;
    object.function = (u32) functions.size();
    object.type.index = 0;
    object.zeroed = false;
    object.read_only = true;
    objects.push_back(object);

    body.object = id;
    body.signature = signature;
    body.form = form;
    functions.push_back(body);

    return id;
}

IrObjectId IrProgram::add_data(const std::string& name, IrTypeId type,
                               const std::vector<u8>& bytes, bool read_only) {
    IrObject object;
    IrObjectId id = {(u32) objects.size()};

    object.kind = IR_OBJECT_DATA;
    object.name = name;
    object.function = 0;
    object.type = type;
    object.bytes = bytes;
    object.zeroed = false;
    object.read_only = read_only;
    objects.push_back(object);

    return id;
}

IrObjectId IrProgram::add_zeroed(const std::string& name, IrTypeId type) {
    IrObjectId id = add_data(name, type, std::vector<u8>(), false);

    objects[id.index].zeroed = true;

    return id;
}

IrObjectId IrProgram::add_text(const std::string& name,
                               const std::string& text) {
    std::vector<u8> bytes(text.begin(), text.end());

    bytes.push_back(0);

    return add_data(name,
                    types.array(types.scalar(IR_B8), (u32) bytes.size()),
                    bytes, true);
}

IrObject& IrProgram::get_object(IrObjectId id) {
    return objects[id.index];
}

u32 IrProgram::get_object_count() {
    return (u32) objects.size();
}

IrFunction& IrProgram::get_function(IrObjectId id) {
    return functions[objects[id.index].function];
}
