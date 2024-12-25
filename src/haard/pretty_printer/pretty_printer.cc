#include <cassert>
#include <iostream>
#include "haard/pretty_printer/pretty_printer.h"

using namespace haard;

PrettyPrinter::PrettyPrinter() {
    indent_level = 0;
}

std::string PrettyPrinter::get_output() {
    return out.str();
}

void PrettyPrinter::print(AstNode* node) {
    if (node == nullptr) {
        return;
    }

    switch (node->get_kind()) {
    case AST_UNKNOWN:
        break;

    case AST_MODULE:
        print_module((Module*) node);
        break;

    /* Import */
    case AST_IMPORT:
        print_import((Import*) node);
        break;

    /* Definitions */
    case AST_CLASS:
        print_class((Class*) node);
        break;

    case AST_ENUM:
    case AST_STRUCT:
    case AST_UNION:
        print_user_type((UserType*) node);
        break;

    case AST_SUPER:
        print_super_type(node);
        break;

    case AST_FUNCTION:
        print_function((Function*) node);
        break;

    case AST_VARIABLE:
        print_variable((Variable*) node);
        break;

    case AST_LAMBDA:
        print_lambda(node);
        break;

    /* Statements */
    case AST_STATEMENTS:
        print_statements((Statements*) node);
        break;

    case AST_WHILE:
        print_while((WhileStatement*) node);
        break;

    case AST_FOR:
        print_for((ForStatement*) node);
        break;

    case AST_FOREACH:
        print_foreach((ForeachStatement*) node);
        break;

    case AST_IF:
        print_if((IfStatement*) node);
        break;

    case AST_ELIF:
        print_elif((ElifStatement*) node);
        break;

    case AST_ELSE:
        print_else((ElseStatement*) node);
        break;

    case AST_RETURN:
        print_return((ReturnStatement*) node);
        break;

    case AST_SWITCH:
        print_switch(node);
        break;

    case AST_SWITCH_BRACE:
        print_switch_brace(node);
        break;

    case AST_SWITCH_CASES:
        print_switch_cases(node);
        break;

    case AST_SWITCH_CASE:
        print_switch_case(node);
        break;

    case AST_SWITCH_DEFAULT:
        print_switch_default(node);
        break;

    case AST_STATEMENT_EXPRESSION:
        print_expression_statement((ExpressionStatement*) node);
        break;

    /* Expressions */
    case AST_ASSIGNMENT:
        print_assignment((Assignment*) node);
        break;

    case AST_DIVISION_ASSIGNMENT:
        print_division_assignment((DivisionAssignment*) node);
        break;

    case AST_TIMES_ASSIGNMENT:
        print_times_assignment((TimesAssignment*) node);
        break;

    case AST_INTEGER_DIVISION_ASSIGNMENT:
        print_integer_division_assignment((IntegerDivisionAssignment*) node);
        break;

    case AST_MODULO_ASSIGNMENT:
        print_modulo_assignment((ModuloAssignment*) node);
        break;

    case AST_MINUS_ASSIGNMENT:
        print_minus_assignment((MinusAssignment*) node);
        break;

    case AST_PLUS_ASSIGNMENT:
        print_plus_assignment((PlusAssignment*) node);
        break;

    case AST_SHIFT_LEFT_LOGICAL_ASSIGNMENT:
        print_shift_left_logical_assignment((ShiftLeftLogicalAssignment*) node);
        break;

    case AST_SHIFT_RIGHT_ARITHMETIC_ASSIGNMENT:
        print_shift_right_arithmetic_assignment((ShiftRightArithmeticAssignment*) node);
        break;

    case AST_SHIFT_RIGHT_LOGICAL_ASSIGNMENT:
        print_shift_right_logical_assignment((ShiftRightLogicalAssignment*) node);
        break;

    case AST_BITWISE_AND_ASSIGNMENT:
        print_bitwise_and_assignment((BitwiseAndAssignment*) node);
        break;

    case AST_BITWISE_XOR_ASSIGNMENT:
        print_bitwise_xor_assignment((BitwiseXorAssignment*) node);
        break;

    case AST_BITWISE_OR_ASSIGNMENT:
        print_bitwise_or_assignment((BitwiseOrAssignment*) node);
        break;

    case AST_BITWISE_NOT_ASSIGNMENT:
        print_bitwise_not_assignment((BitwiseNotAssignment*) node);
        break;

    case AST_CAST:
        print_cast(node);
        break;

    case AST_LOGICAL_OR:
        print_logical_or((LogicalOr*) node);
        break;

    case AST_LOGICAL_AND:
        print_logical_and((LogicalAnd*) node);
        break;

    case AST_EQUAL:
        print_equal((Equal*) node);
        break;

    case AST_NOT_EQUAL:
        print_not_equal((NotEqual*) node);
        break;

    case AST_LESS_THAN:
        print_less_than((LessThan*) node);
        break;

    case AST_LESS_THAN_OR_EQUAL:
        print_less_than_or_equal((LessThanOrEqual*) node);
        break;

    case AST_GREATER_THAN:
        print_greater_than((GreaterThan*) node);
        break;

    case AST_GREATER_THAN_OR_EQUAL:
        print_greater_than_or_equal((GreaterThanOrEqual*) node);
        break;

    case AST_IN:
        print_in((In*) node);
        break;

    case AST_NOT_IN:
        print_not_in((NotIn*) node);
        break;

    case AST_INCLUSIVE_RANGE:
        print_inclusive_range((InclusiveRange*) node);
        break;

    case AST_EXCLUSIVE_RANGE:
        print_exclusive_range((ExclusiveRange*) node);
        break;

    case AST_PLUS:
        print_plus((Plus*) node);
        break;

    case AST_MINUS:
        print_minus((Minus*) node);
        break;

    case AST_TIMES:
        print_times((Times*) node);
        break;

    case AST_DIVISION:
        print_division((Division*) node);
        break;

    case AST_MODULO:
        print_modulo((Modulo*) node);
        break;

    case AST_INTEGER_DIVISION:
        print_integer_division((IntegerDivision*) node);
        break;

    case AST_POWER:
        print_power((Power*) node);
        break;

    case AST_BITWISE_OR:
        print_bitwise_or((BitwiseOr*) node);
        break;

    case AST_BITWISE_XOR:
        print_bitwise_xor((BitwiseXor*) node);
        break;

    case AST_BITWISE_AND:
        print_bitwise_and((BitwiseAnd*) node);
        break;

    case AST_SHIFT_LEFT_LOGICAL:
        print_shift_left_logical((ShiftLeftLogical*) node);
        break;

    case AST_SHIFT_RIGHT_LOGICAL:
        print_shift_right_logical((ShiftRightLogical*) node);
        break;

    case AST_SHIFT_RIGHT_ARITHMETIC:
        print_shift_right_arithmetic((ShiftRightArithmetic*) node);
        break;

    case AST_LOGICAL_NOT:
        print_logical_not((LogicalNot*) node);
        break;

    case AST_ADDRESS_OF:
        print_address_of((AddressOf*) node);
        break;

    case AST_DEREFERENCE:
        print_dereference((Dereference*) node);
        break;

    case AST_BITWISE_NOT:
        print_bitwise_not((BitwiseNot*) node);
        break;

    case AST_UNARY_MINUS:
        print_unary_minus((UnaryMinus*) node);
        break;

    case AST_UNARY_PLUS:
        print_unary_plus((UnaryPlus*) node);
        break;

    case AST_PRE_INCREMENT:
        print_pre_increment((PreIncrement*) node);
        break;

    case AST_PRE_DECREMENT:
        print_pre_decrement((PreDecrement*) node);
        break;

    case AST_SIZEOF:
        print_sizeof(node);
        break;

    case AST_DOT:
        print_dot((Dot*) node);
        break;

    case AST_ARROW:
        print_arrow((Arrow*) node);
        break;

    case AST_INDEX:
        print_index(node);
        break;

    case AST_CALL:
        print_call((Call*) node);
        break;

    case AST_NAMED_ARGUMENT:
        print_named_argument((NamedArgument*) node);
        break;

    case AST_POS_DECREMENT:
        print_pos_decrement(node);
        break;

    case AST_POS_INCREMENT:
        print_pos_increment(node);
        break;

    case AST_PARENTHESIS:
        print_parenthesis((Parenthesis*) node);
        break;

    case AST_NEW:
        print_new((New*) node);
        break;

    case AST_DELETE:
        print_delete((Delete*) node);
        break;

    case AST_DELETE_ARRAY:
        print_delete_array((DeleteArray*) node);
        break;

    case AST_TUPLE:
        print_tuple((Tuple*) node);
        break;

    case AST_SEQUENCE:
        print_sequence((Sequence*) node);
        break;

    case AST_LIST:
        print_list((List*) node);
        break;

    case AST_ARRAY:
        print_array((Array*) node);
        break;

    case AST_HASH:
        print_hash((Hash*) node);
        break;

    case AST_HASH_PAIR:
        print_hash_pair((HashPair*) node);
        break;

    case AST_GENERIC_APPLICATION:
        print_generic_application((GenericsApplication*) node);
        break;

    case AST_SCOPE:
        print_scope((Scope*) node);
        break;

    case AST_IDENTIFIER:
        print_identifier((Identifier*) node);
        break;

    /* Literals */
    case AST_THIS:
        print_this((This*) node);
        break;

    case AST_NULL:
        print_null((Null*) node);
        break;

    case AST_LITERAL_BOOLEAN:
        print_boolean_literal((BooleanLiteral*) node);
        break;

    case AST_LITERAL_CHAR:
        print_char_literal((CharLiteral*) node);
        break;

    case AST_LITERAL_INTEGER:
        print_integer_literal((IntegerLiteral*) node);
        break;

    case AST_LITERAL_FLOAT:
        print_float_literal((FloatLiteral*) node);
        break;

    case AST_LITERAL_STRING:
        print_string_literal((StringLiteral*) node);
        break;

    case AST_LITERAL_SYMBOL:
        out << ':' << node->get_value();
        break;

    /* Types */
    case AST_TYPE_I8:
        out << "i8";
        break;

    case AST_TYPE_U8:
        out << "u8";
        break;

    case AST_TYPE_I16:
        out << "i16";
        break;

    case AST_TYPE_U16:
        out << "u16";
        break;

    case AST_TYPE_I32:
        out << "i32";
        break;

    case AST_TYPE_U32:
        out << "u32";
        break;

    case AST_TYPE_I64:
        out << "i64";
        break;

    case AST_TYPE_U64:
        out << "u64";
        break;

    case AST_TYPE_F32:
        out << "f32";
        break;

    case AST_TYPE_F64:
        out << "f64";
        break;

    case AST_TYPE_CHAR:
        out << "char";
        break;

    case AST_TYPE_VOID:
        out << "void";
        break;

    case AST_TYPE_STR:
        out << "str";
        break;

    case AST_TYPE_SYMBOL:
        out << "sym";
        break;

    case AST_TYPE_BOOL:
        out << "bool";
        break;

    case AST_TYPE_POINTER:
        print_pointer_type((PointerType*) node);
        break;

    case AST_TYPE_REFERENCE:
        print_reference_type((ReferenceType*) node);
        break;

    case AST_TYPE_LIST:
        print_list_type(node);
        break;

    case AST_TYPE_ARRAY:
        print_array_type((ArrayType*) node);
        break;

    case AST_TYPE_TUPLE:
        print_tuple_type((TupleType*) node);
        break;

    case AST_TYPE_FUNCTION:
        print_function_type((FunctionType*) node);
        break;

    case AST_TYPE_NAMED:
        print_named_type((NamedType*) node);
        break;

    /* Others */
    case AST_GENERICS:
        print_generics((Generics*) node);
        break;

    default:
        std::cout << "unhandled case: " << node->get_kind() << "\n";
        break;
    }
}

void PrettyPrinter::print_module(Module* module) {
    AstKind last_kind;
    bool first = true;
    bool needs_extra_newline = false;

    for (auto child : module->get_children()) {
        if (!first) {
            if (last_kind != child->get_kind() || needs_extra_newline) {
                out << "\n";
            }
        }

        print(child);
        out << "\n";

        switch (child->get_kind()) {
        case AST_FUNCTION:
        case AST_CLASS:
        case AST_STRUCT:
        case AST_ENUM:
        case AST_UNION:
            needs_extra_newline = true;
            break;

        default:
            needs_extra_newline = false;
            break;
        }

        first = false;
        last_kind = child->get_kind();
    }
}

void PrettyPrinter::print_class(Class* node) {
    print_user_type(node);
}

void PrettyPrinter::print_import(Import* import) {
    size_t i;

    out << "import ";
    auto path = import->get_path();

    if (path.size() > 0) {
        for (i = 0; i < path.size() - 1; ++i) {
            out << path[i].get_value() << ".";
        }

        out << path[i].get_value();
    }

    if (import->has_alias()) {
        out << " as " << import->get_alias().get_value();
    }
}

void PrettyPrinter::print_user_type(UserType* node) {
    if (node->get_kind() == AST_CLASS) {
        out << "class ";
    } else if (node->get_kind() == AST_ENUM) {
        out << "enum ";
    } else if (node->get_kind() == AST_STRUCT) {
        out << "struct ";
    } else {
        out << "union ";
    }

    out << node->get_name().get_value();

    if (node->get_base_type()) {
        out << "(";
        print(node->get_base_type());
        out << ")";
    }

    print(node->get_generics());

    out << ":\n";
    indent();

    for (auto v : node->get_variables()) {
        print_indentation();
        print(v);
        out << "\n";
    }

    for (auto f : node->get_functions()) {
        print_indentation();
        print(f);
        out << "\n";
    }

    dedent();
}

void PrettyPrinter::print_super_type(AstNode* node) {
    out << "(";
    print(node->get_child());
    out << ")";
}

void PrettyPrinter::print_pointer_type(PointerType* node) {
    print(node->get_subtype());
    out << "*";
}

void PrettyPrinter::print_reference_type(ReferenceType* node) {
    print(node->get_subtype());
    out << "&";
}

void PrettyPrinter::print_list_type(AstNode* node) {
    out << '[';
    print(node->get_child(0));
    out << ']';
}

void PrettyPrinter::print_array_type(ArrayType* type) {
    print(type->get_subtype());
    out << "[";
    print(type->get_expression());
    out << "]";
}

void PrettyPrinter::print_tuple_type(TupleType* tuple) {
    bool first = true;

    out << "(";

    for (auto type : tuple->get_types()) {
        if (!first) out << ", ";
        print(type);
        first = false;
    }

    out << ")";
}

void PrettyPrinter::print_function_type(FunctionType* ftype) {
    bool first = true;

    out << "(";

    for (auto type : ftype->get_parameters_type()) {
        if (!first) out << ", ";
        print(type);
        first = false;
    }

    out << ") -> ";
    print(ftype->get_return_type());
}

void PrettyPrinter::print_named_type(NamedType* type) {
    print(type->get_name_expression());
}

void PrettyPrinter::print_assignment(Assignment* node) {
    print(node->get_left());
    out << " = ";
    print(node->get_right());
}

void PrettyPrinter::print_times_assignment(TimesAssignment* node) {
    print(node->get_left());
    out << " *= ";
    print(node->get_right());
}

void PrettyPrinter::print_division_assignment(DivisionAssignment* node) {
    print(node->get_left());
    out << " / ";
    print(node->get_right());
}

void PrettyPrinter::print_integer_division_assignment(IntegerDivisionAssignment* node) {
    print(node->get_left());
    out << " // ";
    print(node->get_right());
}

void PrettyPrinter::print_modulo_assignment(ModuloAssignment* node) {
    print(node->get_left());
    out << " %= ";
    print(node->get_right());
}

void PrettyPrinter::print_minus_assignment(MinusAssignment* node) {
    print(node->get_left());
    out << " -= ";
    print(node->get_right());
}

void PrettyPrinter::print_plus_assignment(PlusAssignment* node) {
    print(node->get_left());
    out << " += ";
    print(node->get_right());
}

void PrettyPrinter::print_shift_left_logical_assignment(ShiftLeftLogicalAssignment* node) {
    print(node->get_left());
    out << " <<= ";
    print(node->get_right());
}

void PrettyPrinter::print_shift_right_arithmetic_assignment(ShiftRightArithmeticAssignment* node){
    print(node->get_left());
    out << " >>= ";
    print(node->get_right());
}

void PrettyPrinter::print_shift_right_logical_assignment(ShiftRightLogicalAssignment* node) {
    print(node->get_left());
    out << " >>>= ";
    print(node->get_right());
}

void PrettyPrinter::print_bitwise_and_assignment(BitwiseAndAssignment* node) {
    print(node->get_left());
    out << " &= ";
    print(node->get_right());
}

void PrettyPrinter::print_bitwise_xor_assignment(BitwiseXorAssignment* node) {
    print(node->get_left());
    out << " ^= ";
    print(node->get_right());
}

void PrettyPrinter::print_bitwise_or_assignment(BitwiseOrAssignment* node) {
    print(node->get_left());
    out << " |= ";
    print(node->get_right());
}

void PrettyPrinter::print_bitwise_not_assignment(BitwiseNotAssignment* node) {
    print(node->get_left());
    out << " ~= ";
    print(node->get_right());
}

void PrettyPrinter::print_cast(AstNode* node) {
    print_binop(node, "as");
}

void PrettyPrinter::print_logical_or(LogicalOr* node) {
    print(node->get_left());
    out << " " << node->get_token().get_value() << " ";
    print(node->get_right());
}

void PrettyPrinter::print_logical_and(LogicalAnd* node) {
    print(node->get_left());
    out << " " << node->get_token().get_value() << " ";
    print(node->get_right());
}

void PrettyPrinter::print_equal(Equal* node) {
    print(node->get_left());
    out << " == ";
    print(node->get_right());
}

void PrettyPrinter::print_not_equal(NotEqual* node) {
    print(node->get_left());
    out << " != ";
    print(node->get_right());
}

void PrettyPrinter::print_less_than(LessThan* node) {
    print(node->get_left());
    out << " < ";
    print(node->get_right());
}

void PrettyPrinter::print_less_than_or_equal(LessThanOrEqual* node) {
    print(node->get_left());
    out << " <= ";
    print(node->get_right());
}

void PrettyPrinter::print_greater_than(GreaterThan* node) {
    print(node->get_left());
    out << " > ";
    print(node->get_right());
}

void PrettyPrinter::print_greater_than_or_equal(GreaterThanOrEqual* node) {
    print(node->get_left());
    out << " >= ";
    print(node->get_right());
}

void PrettyPrinter::print_in(In* node) {
    print(node->get_left());
    out << " in ";
    print(node->get_right());
}

void PrettyPrinter::print_not_in(NotIn* node) {
    print(node->get_left());
    out << " not in ";
    print(node->get_right());
}

void PrettyPrinter::print_inclusive_range(InclusiveRange* node) {
    print(node->get_left());
    out << " .. ";
    print(node->get_right());
}

void PrettyPrinter::print_exclusive_range(ExclusiveRange* node) {
    print(node->get_left());
    out << " ... ";
    print(node->get_right());
}

void PrettyPrinter::print_plus(Plus* node) {
    print(node->get_left());
    out << " + ";
    print(node->get_right());
}

void PrettyPrinter::print_minus(Minus* node) {
    print(node->get_left());
    out << " - ";
    print(node->get_right());
}

void PrettyPrinter::print_times(Times* node) {
    print(node->get_left());
    out << " * ";
    print(node->get_right());
}

void PrettyPrinter::print_division(Division* node) {
    print(node->get_left());
    out << " / ";
    print(node->get_right());
}

void PrettyPrinter::print_modulo(Modulo* node) {
    print(node->get_left());
    out << " % ";
    print(node->get_right());
}

void PrettyPrinter::print_integer_division(IntegerDivision* node) {
    print(node->get_left());
    out << " // ";
    print(node->get_right());
}

void PrettyPrinter::print_power(Power* node) {
    print(node->get_left());
    out << " ** ";
    print(node->get_right());
}

void PrettyPrinter::print_bitwise_or(BitwiseOr* node) {
    print(node->get_left());
    out << " | ";
    print(node->get_right());
}

void PrettyPrinter::print_bitwise_xor(BitwiseXor* node) {
    print(node->get_left());
    out << " ^ ";
    print(node->get_right());
}

void PrettyPrinter::print_bitwise_and(BitwiseAnd* node) {
    print(node->get_left());
    out << " & ";
    print(node->get_right());
}

void PrettyPrinter::print_shift_left_logical(ShiftLeftLogical* node) {
    print(node->get_left());
    out << " << ";
    print(node->get_right());
}

void PrettyPrinter::print_shift_right_logical(ShiftRightLogical* node) {
    print(node->get_left());
    out << " >>> ";
    print(node->get_right());
}

void PrettyPrinter::print_shift_right_arithmetic(ShiftRightArithmetic* node) {
    print(node->get_left());
    out << " >> ";
    print(node->get_right());
}

void PrettyPrinter::print_dot(Dot* node) {
    print(node->get_left());
    out << ".";
    print(node->get_right());
}

void PrettyPrinter::print_arrow(Arrow* node) {
    print(node->get_left());
    out << "->";
    print(node->get_right());
}

void PrettyPrinter::print_parenthesis(Parenthesis* node) {
    out << "(";
    print(node->get_expression());
    out << ")";
}

void PrettyPrinter::print_generic_application(GenericsApplication* node) {
    print(node->get_expression());
    print(node->get_generics());
}

void PrettyPrinter::print_scope(Scope* scope) {
    print(scope->get_alias());
    out << "::";
    print(scope->get_name());
}

void PrettyPrinter::print_identifier(Identifier* id) {
    out << id->get_token().get_value();
}

void PrettyPrinter::print_this(This* node) {
    out << "this";
}

void PrettyPrinter::print_null(Null* node) {
    out << "null";
}

void PrettyPrinter::print_boolean_literal(BooleanLiteral* node) {
    out << node->get_token().get_value();
}

void PrettyPrinter::print_char_literal(CharLiteral* node) {
    out << node->get_token().get_value();
}

void PrettyPrinter::print_integer_literal(IntegerLiteral* node) {
    out << node->get_token().get_value();
}

void PrettyPrinter::print_float_literal(FloatLiteral* node) {
    out << node->get_token().get_value();
}

void PrettyPrinter::print_string_literal(StringLiteral* node) {
    out << node->get_token().get_value();
}

void PrettyPrinter::print_function(Function* function) {
    bool has_parameters = false;

    print_indentation();
    out << "def " << function->get_name().get_value();

    print(function->get_generics());

    out << " : ";
    print(function->get_return_type());
    out << '\n';
    indent();

    for (auto p : function->get_parameters()) {
        print_indentation();
        print(p);
        out << "\n";
        has_parameters = true;
    }

    if (has_parameters) {
        out << "\n";
    }

    print(function->get_statements());
    dedent();
}

void PrettyPrinter::print_variable(Variable* var) {
    AstKind kind = var->get_parent()->get_kind();

    if (kind == AST_FUNCTION) {
        out << "@";
    } else if (kind == AST_STATEMENTS || kind == AST_MODULE) {
        out << "var ";
    }

    out << var->get_name().get_value();

    if (var->get_type()) {
        out << " : ";
        print(var->get_type());
    }

    if (var->get_expression()) {
        out << " = ";
        print(var->get_expression());
    }
}

void PrettyPrinter::print_lambda(AstNode* node) {
    out << "|";

    auto params = node->get_children(AST_VARIABLE);

    if (params.size() > 0) {
        int i = 0;

        for (i = 0; i < params.size() - 1; ++i) {
            //print_variable(params[i]);
            out << ", ";
        }

        //print_variable(params[i]);
    }

    out << "|";

    if (node->get_child(AST_TYPE)) {
        out << " -> ";
        print(node->get_child(AST_TYPE)->get_child());
    }

    out << " ";

    out << "{\n";
    indent();
    print(node->get_child(AST_STATEMENTS));
    dedent();
    print_indentation();
    out << "}";
}

void PrettyPrinter::print_while(WhileStatement* stmt) {
    out << "while ";
    print(stmt->get_expression());
    out << ":\n";
    indent();
    print(stmt->get_statements());
    dedent();
}

void PrettyPrinter::print_for(ForStatement* stmt) {
    bool first = true;

    out << "for ";

    for (auto e : stmt->get_initialization()) {
        if (!first) out << ", ";
        print(e);
        first = false;
    }

    out << "; ";
    print(stmt->get_test());
    out << "; ";

    first = true;
    for (auto e : stmt->get_update()) {
        if (!first) out << ", ";
        print(e);
        first = false;
    }

    out << ":\n";
    indent();
    print(stmt->get_statements());
    dedent();
}

void PrettyPrinter::print_foreach(ForeachStatement* stmt) {
    out << "for ";
    print(stmt->get_expression());
    out << ":\n";
    indent();
    print(stmt->get_statements());
    dedent();
}

void PrettyPrinter::print_if(IfStatement* node) {
    out << "if ";
    print(node->get_expression());
    out << ":\n";
    indent();
    print(node->get_true_statements());
    dedent();
    print(node->get_false_statements());
}

void PrettyPrinter::print_elif(ElifStatement* node) {
    out << "\n";
    print_indentation();
    out << "elif ";
    print(node->get_expression());
    out << ":\n";
    indent();
    print(node->get_true_statements());
    dedent();
    print(node->get_false_statements());
}

void PrettyPrinter::print_else(ElseStatement* node) {
    out << "\n";
    print_indentation();
    out << "else:\n";
    indent();
    print(node->get_statements());
    dedent();
}

/* Statements */
void PrettyPrinter::print_statements(Statements* stmts) {
    bool no_statements = true;
    bool first = true;

    for (auto stmt : stmts->get_statements()) {
        if (!first) {
            out << "\n";
        }

        print_indentation();
        print(stmt);
        no_statements = false;
        first = false;
    }

    if (no_statements) {
        print_indentation();
        out << "pass";
    }
}

void PrettyPrinter::print_return(ReturnStatement* node) {
    out << "return";

    if (node->get_expression()) {
        out << " ";
        print(node->get_expression());
    }
}

void PrettyPrinter::print_switch(AstNode* node) {
    out << "switch ";
    print(node->get_child(0));
    out << ":\n";
    indent();

    for (int i = 1; i < node->children_count(); ++i) {
        print(node->get_child(i));
    }

    dedent();
}

void PrettyPrinter::print_switch_brace(AstNode* node) {
    print(node->get_child(0));

    indent();
    print(node->get_child(1));
    dedent();
    out << "\n";
}

void PrettyPrinter::print_switch_cases(AstNode* node) {
    for (int i = 0; i < node->children_count(); ++i) {
        print(node->get_child(i));
        out << "\n";
    }
}

void PrettyPrinter::print_switch_case(AstNode* node) {
    print_indentation();
    out << "case ";
    print(node->get_child());
    out << ":";
}

void PrettyPrinter::print_switch_default(AstNode* node) {
    print_indentation();
    out << "default:\n";
    indent();
    print(node->get_child());
    dedent();
    out << "\n";
}

void PrettyPrinter::print_expression_statement(ExpressionStatement* stmt) {
    print(stmt->get_expression());
}

void PrettyPrinter::print_index(AstNode* node) {
    print(node->get_child(0));
    out << '[';

    print(node->get_child(1));
    out << ']';
}

void PrettyPrinter::print_hash_pair(HashPair* pair) {
    print(pair->get_left());
    out << ": ";
    print(pair->get_right());
}

void PrettyPrinter::print_logical_not(LogicalNot* un) {
    if (un->get_token().get_kind() == TK_LOGICAL_NOT) {
        out << "!";
    } else {
        out << "not ";
    }

    print(un->get_expression());
}

void PrettyPrinter::print_address_of(AddressOf* node) {
    out << "&";
    print(node->get_expression());
}

void PrettyPrinter::print_dereference(Dereference* node) {
    out << "*";
    print(node->get_expression());
}

void PrettyPrinter::print_bitwise_not(BitwiseNot* node) {
    out << "~";
    print(node->get_expression());
}

void PrettyPrinter::print_unary_minus(UnaryMinus* node) {
    out << "-";
    print(node->get_expression());
}

void PrettyPrinter::print_unary_plus(UnaryPlus* node) {
    out << "+";
    print(node->get_expression());
}

void PrettyPrinter::print_pre_increment(PreIncrement* node) {
    out << "++";
    print(node->get_expression());
}

void PrettyPrinter::print_pre_decrement(PreDecrement* node) {
    out << "--";
    print(node->get_expression());
}

void PrettyPrinter::print_call(Call* node) {
    print(node->get_expression());
    out << '(';

    bool first = true;
    for (auto expr : node->get_arguments()) {
        if (!first) out << ", ";

        print(expr);
        first = false;
    }

    out << ')';
}

void PrettyPrinter::print_named_argument(NamedArgument* node) {
    out << node->get_name().get_value() << ": ";
    print(node->get_expression());
}

void PrettyPrinter::print_pos_increment(AstNode* node) {
    print(node->get_child(0));
    out << "++";
}

void PrettyPrinter::print_pos_decrement(AstNode* node) {
    print(node->get_child(0));
    out << "--";
}

void PrettyPrinter::print_sizeof(AstNode* node) {
    out << "sizeof(";
    print(node->get_child());
    out << ")";
}

void PrettyPrinter::print_new(New* node) {
    out << "new ";
    print(node->get_type());

    bool first = true;

    for (auto expr : node->get_arguments()) {
        if (first) out << "(";
        if (!first) out << ", ";
        print(expr);
        first = false;
    }

    if (!first) {
        out << ")";
    }
}

void PrettyPrinter::print_delete(Delete* node) {
    out << "delete ";
    print(node->get_expression());
}

void PrettyPrinter::print_delete_array(DeleteArray* node) {
    out << "delete[] ";
    print(node->get_expression());
}

void PrettyPrinter::print_type_list(AstNode* tlist, const char* begin, const char* end) {
    if (tlist == nullptr) {
        return;
    }

    int i;

    out << begin;

    for (i = 0; i < tlist->children_count() - 1; ++i) {
        print(tlist->get_child(i));
        out << ", ";
    }

    print(tlist->get_child(i));
    out << end;
}

void PrettyPrinter::print_generics(const Generics* g) {
    out << "<";
    bool first = true;

    for (auto t : g->get_types()) {
        if (!first) out << ", ";
        out << t->to_str();
        first = false;
    }

    out << ">";
}

void PrettyPrinter::print_tuple(Tuple* node) {
    bool first = true;

    out << "(";

    for (auto expr : node->get_expressions()) {
        if (!first) out << ", ";
        print(expr);
        first = false;
    }

    out << ")";
}

void PrettyPrinter::print_sequence(Sequence* node) {
    bool first = true;

    out << "(";

    for (auto expr : node->get_expressions()) {
        if (!first) out << "; ";
        print(expr);
        first = false;
    }

    out << ")";
}

void PrettyPrinter::print_list(List* node) {
    bool first = true;

    out << "[";

    for (auto expr : node->get_expressions()) {
        if (!first) out << ", ";
        print(expr);
        first = false;
    }

    out << "]";
}

void PrettyPrinter::print_array(Array* node) {
    bool first = true;

    out << "{";

    for (auto expr : node->get_expressions()) {
        if (!first) out << ", ";
        print(expr);
        first = false;
    }

    out << "}";
}

void PrettyPrinter::print_hash(Hash* node) {
    bool first = true;

    out << "{";

    for (auto expr : node->get_expressions()) {
        if (!first) out << ", ";
        print(expr);
        first = false;
    }

    out << "}";
}

void PrettyPrinter::indent() {
    ++indent_level;
}

void PrettyPrinter::dedent() {
    --indent_level;
}

void PrettyPrinter::print_indentation() {
    for (int i = 0; i < indent_level; ++i) {
        out << "    ";
    }
}

void PrettyPrinter::print_binop(AstNode* node, const char* oper, bool no_space) {
    //out << "(";
    print(node->get_child(0));

    if (no_space) {
        out << oper;
    } else {
        out << " " << oper << " ";
    }

    print(node->get_child(1));
    //out << ")";
}
