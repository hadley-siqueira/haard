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

void PrettyPrinter::print(Ast* node) {
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
    case AST_ENUM:
    case AST_STRUCT:
    case AST_UNION:
        print_user_type(node);
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
        print_while(node);
        break;

    case AST_FOR:
        print_for(node);
        break;

    case AST_FOR_INIT:
        print_for_init(node);
        break;

    case AST_FOR_UPDATE:
        print_for_update(node);
        break;

    case AST_IF:
        print_if(node);
        break;

    case AST_ELIF:
        print_elif(node);
        break;

    case AST_ELSE:
        print_else(node);
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

    /* Expressions */
    case AST_EXPRESSION:
        print_expression_statement(node);
        break;

    case AST_EXPRESSION_WITH_SEMICOLON:
        print_expression_statement(node, true);
        break;

    case AST_ASSIGNMENT:
        print_assignment(node);
        break;

    case AST_DIVISION_ASSIGNMENT:
        print_division_assignment(node);
        break;

    case AST_TIMES_ASSIGNMENT:
        print_times_assignment(node);
        break;

    case AST_INTEGER_DIVISION_ASSIGNMENT:
        print_integer_division_assignment(node);
        break;

    case AST_MODULO_ASSIGNMENT:
        print_modulo_assignment(node);
        break;

    case AST_MINUS_ASSIGNMENT:
        print_minus_assignment(node);
        break;

    case AST_PLUS_ASSIGNMENT:
        print_plus_assignment(node);
        break;

    case AST_SHIFT_LEFT_LOGICAL_ASSIGNMENT:
        print_shift_left_logical_assignment(node);
        break;

    case AST_SHIFT_RIGHT_ARITHMETIC_ASSIGNMENT:
        print_shift_right_arithmetic_assignment(node);
        break;

    case AST_SHIFT_RIGHT_LOGICAL_ASSIGNMENT:
        print_shift_right_logical_assignment(node);
        break;

    case AST_BITWISE_AND_ASSIGNMENT:
        print_bitwise_and_assignment(node);
        break;

    case AST_BITWISE_XOR_ASSIGNMENT:
        print_bitwise_xor_assignment(node);
        break;

    case AST_BITWISE_OR_ASSIGNMENT:
        print_bitwise_or_assignment(node);
        break;

    case AST_BITWISE_NOT_ASSIGNMENT:
        print_bitwise_not_assignment(node);
        break;

    case AST_CAST:
        print_cast(node);
        break;

    case AST_LOGICAL_OR:
        print_logical_or(node);
        break;

    case AST_OR:
        print_or(node);
        break;

    case AST_LOGICAL_AND:
        print_logical_and(node);
        break;

    case AST_AND:
        print_and(node);
        break;

    case AST_EQUAL:
        print_equal(node);
        break;

    case AST_NOT_EQUAL:
        print_not_equal(node);
        break;

    case AST_LESS_THAN:
        print_less_than(node);
        break;

    case AST_LESS_THAN_OR_EQUAL:
        print_less_than_or_equal(node);
        break;

    case AST_GREATER_THAN:
        print_greater_than(node);
        break;

    case AST_GREATER_THAN_OR_EQUAL:
        print_greater_than_or_equal(node);
        break;

    case AST_IN:
        print_in(node);
        break;

    case AST_NOT_IN:
        print_not_in(node);
        break;

    case AST_INCLUSIVE_RANGE:
        print_inclusive_range(node);
        break;

    case AST_EXCLUSIVE_RANGE:
        print_exclusive_range(node);
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
        print_bitwise_xor(node);
        break;

    case AST_BITWISE_AND:
        print_bitwise_and(node);
        break;

    case AST_SHIFT_LEFT_LOGICAL:
        print_shift_left_logical(node);
        break;

    case AST_SHIFT_RIGHT_LOGICAL:
        print_shift_right_logical(node);
        break;

    case AST_SHIFT_RIGHT_ARITHMETIC:
        print_shift_right_arithmetic(node);
        break;

    case AST_LOGICAL_NOT:
        print_logical_not(node);
        break;

    case AST_NOT:
        print_not(node);
        break;

    case AST_ADDRESS_OF:
        print_address_of(node);
        break;

    case AST_DEREFERENCE:
        print_dereference(node);
        break;

    case AST_BITWISE_NOT:
        print_bitwise_not(node);
        break;

    case AST_UNARY_MINUS:
        print_unary_minus(node);
        break;

    case AST_UNARY_PLUS:
        print_unary_plus(node);
        break;

    case AST_PRE_INCREMENT:
        print_pre_increment(node);
        break;

    case AST_PRE_DECREMENT:
        print_pre_decrement(node);
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
        print_call(node);
        break;

    case AST_ARGUMENTS:
        print_arguments(node);
        break;

    case AST_NAMED_ARGUMENT:
        print_argument_name(node);
        break;

    case AST_POS_DECREMENT:
        print_pos_decrement(node);
        break;

    case AST_POS_INCREMENT:
        print_pos_increment(node);
        break;

    case AST_PARENTHESIS:
        print_parenthesis(node);
        break;

    case AST_TUPLE:
        print_tuple(node);
        break;

    case AST_SEQUENCE:
        print_sequence(node);
        break;

    case AST_LIST:
        print_list(node);
        break;

    case AST_ARRAY:
        print_array(node);
        break;

    case AST_HASH:
        print_hash(node);
        break;

    case AST_HASH_PAIR:
        print_hash_pair(node);
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
        out << "this";
        break;

    case AST_NULL:
        out << "null";
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

void PrettyPrinter::print_user_type(Ast* node) {
    if (node->get_kind() == AST_CLASS) {
        out << "class ";
    } else if (node->get_kind() == AST_ENUM) {
        out << "enum ";
    } else if (node->get_kind() == AST_STRUCT) {
        out << "struct ";
    } else {
        out << "union ";
    }

    out << node->get_value();

    print(node->get_child(AST_SUPER));
    print(node->get_child(AST_GENERICS));

    out << ":\n";
    indent();

    for (auto v : node->get_children(AST_VARIABLE)) {
        print_indentation();
        print(v);
        out << "\n";
    }

    for (auto f : node->get_children(AST_FUNCTION)) {
        print_indentation();
        print(f);
        out << "\n";
    }

    dedent();
}

void PrettyPrinter::print_super_type(Ast* node) {
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

void PrettyPrinter::print_list_type(Ast* node) {
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

void PrettyPrinter::print_assignment(Ast* node) {
    print_binop(node, "=");
}

void PrettyPrinter::print_times_assignment(Ast* node) {
    print_binop(node, "*=");
}

void PrettyPrinter::print_division_assignment(Ast* node) {
    print_binop(node, "/=");
}

void PrettyPrinter::print_integer_division_assignment(Ast* node) {
    print_binop(node, "//=");
}

void PrettyPrinter::print_modulo_assignment(Ast* node) {
    print_binop(node, "%=");
}

void PrettyPrinter::print_minus_assignment(Ast* node) {
    print_binop(node, "-=");
}

void PrettyPrinter::print_plus_assignment(Ast* node) {
    print_binop(node, "+=");
}

void PrettyPrinter::print_shift_left_logical_assignment(Ast* node) {
    print_binop(node, "<<=");
}

void PrettyPrinter::print_shift_right_arithmetic_assignment(Ast* node){
    print_binop(node, ">>=");
}

void PrettyPrinter::print_shift_right_logical_assignment(Ast* node) {
    print_binop(node, ">>>=");
}

void PrettyPrinter::print_bitwise_and_assignment(Ast* node) {
    print_binop(node, "&=");
}

void PrettyPrinter::print_bitwise_xor_assignment(Ast* node) {
    print_binop(node, "^=");
}

void PrettyPrinter::print_bitwise_or_assignment(Ast* node) {
    print_binop(node, "|=");
}

void PrettyPrinter::print_bitwise_not_assignment(Ast* node) {
    print_binop(node, "~=");
}

void PrettyPrinter::print_cast(Ast* node) {
    print_binop(node, "as");
}

void PrettyPrinter::print_logical_or(Ast* node) {
    print_binop(node, "||");
}

void PrettyPrinter::print_or(Ast* node) {
    print_binop(node, "or");
}

void PrettyPrinter::print_logical_and(Ast* node) {
    print_binop(node, "&&");
}

void PrettyPrinter::print_and(Ast* node) {
    print_binop(node, "and");
}

void PrettyPrinter::print_equal(Ast* node) {
    print_binop(node, "==");
}

void PrettyPrinter::print_not_equal(Ast* node) {
    print_binop(node, "!=");
}

void PrettyPrinter::print_less_than(Ast* node) {
    print_binop(node, "<");
}

void PrettyPrinter::print_less_than_or_equal(Ast* node) {
    print_binop(node, "<=");
}

void PrettyPrinter::print_greater_than(Ast* node) {
    print_binop(node, ">");
}

void PrettyPrinter::print_greater_than_or_equal(Ast* node) {
    print_binop(node, ">=");
}

void PrettyPrinter::print_in(Ast* node) {
    print_binop(node, "in");
}

void PrettyPrinter::print_not_in(Ast* node) {
    print_binop(node, "not in");
}

void PrettyPrinter::print_inclusive_range(Ast* node) {
    print_binop(node, "..");
}

void PrettyPrinter::print_exclusive_range(Ast* node) {
    print_binop(node, "...");
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

void PrettyPrinter::print_modulo(Modulo *node) {
    print(node->get_left());
    out << " % ";
    print(node->get_right());
}

void PrettyPrinter::print_integer_division(IntegerDivision *node) {
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

void PrettyPrinter::print_bitwise_xor(Ast* node) {
    print_binop(node, "^");
}

void PrettyPrinter::print_bitwise_and(Ast* node) {
    print_binop(node, "&");
}

void PrettyPrinter::print_shift_left_logical(Ast* node) {
    print_binop(node, "<<");
}

void PrettyPrinter::print_shift_right_logical(Ast* node) {
    print_binop(node, ">>>");
}

void PrettyPrinter::print_shift_right_arithmetic(Ast* node) {
    print_binop(node, ">>");
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

void PrettyPrinter::print_parenthesis(Ast* node) {
    out << "(";
    print(node->get_child(0));
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

void PrettyPrinter::print_lambda(Ast* node) {
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

void PrettyPrinter::print_while(Ast* stmt) {
    out << "while ";
    print(stmt->get_child(0));
    out << ":\n";
    indent();
    print(stmt->get_child(1));
    dedent();
}

void PrettyPrinter::print_for(Ast* stmt) {
    Ast* tmp;

    out << "for ";

    tmp = stmt->get_child(AST_FOR_RANGE);

    if (tmp) {
        print(tmp->get_child());
    } else {
        print(stmt->get_child(AST_FOR_INIT));
        out << "; ";

        tmp = stmt->get_child(AST_FOR_TEST);
        print(tmp->get_child());
        out << "; ";

        print(stmt->get_child(AST_FOR_UPDATE));
    }

    out << ":\n";
    indent();
    print(stmt->get_child(AST_STATEMENTS));
    dedent();
}

void PrettyPrinter::print_for_init(Ast* node) {
    print_expression_list(node, "", "");
}

void PrettyPrinter::print_for_update(Ast* node) {
    print_expression_list(node, "", "");
}

void PrettyPrinter::print_if(Ast* node) {
    print_indentation();
    out << "if ";
    print(node->get_child(0));
    out << ":\n";
    indent();
    print(node->get_child(1));
    dedent();

    print(node->get_child(2));
    /*if (node->get_child(2)) {

    }*/
}

void PrettyPrinter::print_elif(Ast* node) {
    print_indentation();
    out << "elif ";
    print(node->get_child(0));
    out << ":\n";
    indent();
    print(node->get_child(1));
    dedent();

    print(node->get_child(2));
}

void PrettyPrinter::print_else(Ast* node) {
    print_indentation();
    out << "else:\n";
    indent();
    print(node->get_child());
    dedent();
}

/* Statements */
void PrettyPrinter::print_statements(Statements* stmts) {
    bool no_statements = true;

    for (auto stmt : stmts->get_statements()) {
        print_indentation();
        print(stmt);
        no_statements = false;
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

void PrettyPrinter::print_switch(Ast* node) {
    out << "switch ";
    print(node->get_child(0));
    out << ":\n";
    indent();

    for (int i = 1; i < node->children_count(); ++i) {
        print(node->get_child(i));
    }

    dedent();
}

void PrettyPrinter::print_switch_brace(Ast* node) {
    print(node->get_child(0));

    indent();
    print(node->get_child(1));
    dedent();
    out << "\n";
}

void PrettyPrinter::print_switch_cases(Ast* node) {
    for (int i = 0; i < node->children_count(); ++i) {
        print(node->get_child(i));
        out << "\n";
    }
}

void PrettyPrinter::print_switch_case(Ast* node) {
    print_indentation();
    out << "case ";
    print(node->get_child());
    out << ":";
}

void PrettyPrinter::print_switch_default(Ast* node) {
    print_indentation();
    out << "default:\n";
    indent();
    print(node->get_child());
    dedent();
    out << "\n";
}

void PrettyPrinter::print_expression_statement(Ast* stmt, bool has_semicolon) {
    print(stmt->get_child(0));

    if (has_semicolon) {
        out << ";";
    }
}

void PrettyPrinter::print_index(Ast* node) {
    print(node->get_child(0));
    out << '[';

    print(node->get_child(1));
    out << ']';
}

void PrettyPrinter::print_hash_pair(Ast* pair) {
    print(pair->get_child(0));
    out << ": ";
    print(pair->get_child(1));
}

void PrettyPrinter::print_logical_not(Ast* un) {
    out << "!";
    print(un->get_child(0));
}

void PrettyPrinter::print_not(Ast* un) {
    out << "not ";
    print(un->get_child(0));
}

void PrettyPrinter::print_address_of(Ast* node) {
    out << "&";
    print(node->get_child());
}

void PrettyPrinter::print_dereference(Ast* node) {
    out << "*";
    print(node->get_child());
}

void PrettyPrinter::print_bitwise_not(Ast* node) {
    out << "~";
    print(node->get_child());
}

void PrettyPrinter::print_unary_minus(Ast* node) {
    out << "-";
    print(node->get_child());
}

void PrettyPrinter::print_unary_plus(Ast* node) {
    out << "+";
    print(node->get_child());
}

void PrettyPrinter::print_pre_increment(Ast* node) {
    out << "++";
    print(node->get_child());
}

void PrettyPrinter::print_pre_decrement(Ast* node) {
    out << "--";
    print(node->get_child());
}

void PrettyPrinter::print_call(Ast* expr) {
    print(expr->get_child(0));
    print(expr->get_child(1));
}

void PrettyPrinter::print_arguments(Ast* args) {
    print_expression_list(args, "(", ")");
}

void PrettyPrinter::print_argument_name(Ast* node) {
    out << node->get_value() << ": ";
    print(node->get_child());
}

void PrettyPrinter::print_pos_increment(Ast* node) {
    print(node->get_child(0));
    out << "++";
}

void PrettyPrinter::print_pos_decrement(Ast* node) {
    print(node->get_child(0));
    out << "--";
}

void PrettyPrinter::print_sizeof(Ast* node) {
    out << "sizeof(";
    print(node->get_child());
    out << ")";
}

void PrettyPrinter::print_type_list(Ast* tlist, const char* begin, const char* end) {
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

void PrettyPrinter::print_tuple(Ast* expr) {
    print_expression_list(expr, "(", ")");
}

void PrettyPrinter::print_sequence(Ast* expr) {
    print_expression_list(expr, "(", ")", ";");
}

void PrettyPrinter::print_list(Ast* expr) {
    print_expression_list(expr, "[", "]");
}

void PrettyPrinter::print_array(Ast* expr) {
    print_expression_list(expr, "{", "}");
}

void PrettyPrinter::print_hash(Ast* expr) {
    print_expression_list(expr, "{", "}");
}

void PrettyPrinter::print_expression_list(Ast* list, const char* begin, const char* end, const char* sep) {
    if (list == nullptr) {
        return;
    }

    int i;
    out << begin;

    if (list->children_count() > 0) {
        for (i = 0; i < list->children_count() - 1; ++i) {
            print(list->get_child(i));
            out << sep << " ";
        }

        print(list->get_child(i));
    }

    out << end;
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

void PrettyPrinter::print_binop(Ast* node, const char* oper, bool no_space) {
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
