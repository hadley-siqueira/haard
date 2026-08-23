#include <iostream>
#include <haard/pretty_printer/pretty_printer.h>

using namespace haard;

// Whether writing 'second' straight after 'first' would be read back as one
// token that means something else. Only prefix operators can meet with nothing
// between them — every binary operator here carries its own spaces — so the
// question is only ever about two prefix operators in a row, and of those only
// three double into a token with a different meaning:
//
//   '-' '-' -> '--', a pre-decrement, not a negated negation
//   '+' '+' -> '++', a pre-increment, not a doubled unary plus
//   '&' '&' -> '&&', a logical and, not the address of an address
//
// '*' '*' is deliberately **not** in the list: '**' is the language's own
// spelling for two dereferences, so pasting them changes nothing. Neither are
// pairs like '~' '-', which no token is made of.
//
// This list has to grow when a prefix operator does — a '|' one, for the
// closure syntax, would need it.
static bool would_paste(char first, char second) {
    if (first != second) {
        return false;
    }

    return first == '-' || first == '+' || first == '&';
}

PrettyPrinter::PrettyPrinter() {
    context = nullptr;
    ast = nullptr;
    indentation = 0;
    last_character = 0;
}

bool PrettyPrinter::print(std::ostream& out) {
    u32 root = ast->get_root();

    if (root == 0) {
        return false;
    }

    output.str("");
    indentation = 0;
    last_character = 0;
    print_node(root);
    out << output.str() << '\n';

    return true;
}

void PrettyPrinter::print_node(u32 node) {
    if (node == 0) {
        return;
    }

    auto kind = ast->get_node(node)->get_kind();

    switch (kind) {
        case AST_MODULE:
            print_module(node);
            break;

        case AST_IMPORT:
            print_import(node);
            break;

        case AST_IMPORT_PATH:
            print_import_path(node);
            break;

        case AST_IMPORT_PATH_SEGMENT:
            print_import_path_segment(node);
            break;

        case AST_IMPORT_ALIAS:
            print_import_alias(node);
            break;

        case AST_LET_DECLARATION:
            print_let_declaration(node);
            break;

        case AST_CONST_DECLARATION:
            print_const_declaration(node);
            break;

        case AST_FUNCTION:
            print_function(node);
            break;

        case AST_GENERIC_PARAMETERS:
            print_generic_parameters(node);
            break;

        case AST_FUNCTION_RETURN_TYPE:
            print_function_return_type(node);
            break;

        case AST_BLOCK:
            print_block(node);
            break;

        case AST_PASS:
            print_pass(node);
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

        case AST_WHILE:
            print_while(node);
            break;

        case AST_RETURN:
            print_return(node);
            break;

        case AST_BREAK:
            print_break(node);
            break;

        case AST_CONTINUE:
            print_continue(node);
            break;

        case AST_YIELD:
            print_yield(node);
            break;

        case AST_GOTO:
            print_goto(node);
            break;

        case AST_PARAM:
            print_param(node);
            break;

        case AST_BINDING:
            print_binding(node);
            break;

        case AST_BINDING_NAME:
            print_binding_name(node);
            break;

        case AST_BINDING_TYPE:
            print_binding_type(node);
            break;

        case AST_BINDING_EXPRESSION:
            print_binding_expression(node);
            break;

        case AST_PLUS:
            print_plus_expression(node);
            break;

        case AST_MINUS:
            print_minus_expression(node);
            break;

        case AST_TIMES:
            print_times_expression(node);
            break;

        case AST_DIVISION:
            print_division_expression(node);
            break;

        case AST_INTEGER_DIVISION:
            print_integer_division_expression(node);
            break;

        case AST_MODULO:
            print_modulo_expression(node);
            break;

        // every assignment writes its own lexeme, so one printer serves the
        // twelve of them
        case AST_ASSIGNMENT:
        case AST_PLUS_ASSIGNMENT:
        case AST_MINUS_ASSIGNMENT:
        case AST_TIMES_ASSIGNMENT:
        case AST_DIVISION_ASSIGNMENT:
        case AST_MODULO_ASSIGNMENT:
        case AST_BITWISE_AND_ASSIGNMENT:
        case AST_BITWISE_OR_ASSIGNMENT:
        case AST_BITWISE_XOR_ASSIGNMENT:
        case AST_BITWISE_NOT_ASSIGNMENT:
        case AST_INTEGER_DIVISION_ASSIGNMENT:
        case AST_BITWISE_LEFT_SHIFT_ASSIGNMENT:
        case AST_BITWISE_RIGHT_SHIFT_ASSIGNMENT:
        case AST_BITWISE_UNSIGNED_RIGHT_SHIFT_ASSIGNMENT:
            print_assignment_expression(node);
            break;

        case AST_CAST:
            print_cast_expression(node);
            break;

        // 'or' / '||' and 'and' / '&&' are one operator with two spellings
        case AST_LOGICAL_OR:
        case AST_LOGICAL_AND:
            print_logical_expression(node);
            break;

        case AST_EQUAL:
        case AST_NOT_EQUAL:
        case AST_LESS_THAN:
        case AST_GREATER_THAN:
        case AST_LESS_THAN_OR_EQUAL:
        case AST_GREATER_THAN_OR_EQUAL:
        case AST_IN:
            print_comparison_expression(node);
            break;

        case AST_NOT_IN:
            print_not_in_expression(node);
            break;

        case AST_INCLUSIVE_RANGE:
        case AST_EXCLUSIVE_RANGE:
            print_range_expression(node);
            break;

        case AST_POWER:
            print_power_expression(node);
            break;

        case AST_BITWISE_OR:
            print_bitwise_or_expression(node);
            break;

        case AST_BITWISE_XOR:
            print_bitwise_xor_expression(node);
            break;

        case AST_BITWISE_AND:
            print_bitwise_and_expression(node);
            break;

        case AST_BITWISE_LEFT_SHIFT:
            print_left_shift_expression(node);
            break;

        case AST_BITWISE_RIGHT_SHIFT:
            print_right_shift_expression(node);
            break;

        case AST_BITWISE_UNSIGNED_RIGHT_SHIFT:
            print_unsigned_right_shift_expression(node);
            break;

        case AST_LOGICAL_NOT:
            print_logical_not(node);
            break;

        case AST_LOGICAL_NOT_OPERATOR:
            print_logical_not_operator(node);
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

        case AST_SCOPE:
            print_scope(node);
            break;

        case AST_DOT:
            print_dot(node);
            break;

        case AST_ARROW:
            print_arrow(node);
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

        case AST_POST_INCREMENT:
            print_post_increment(node);
            break;

        case AST_POST_DECREMENT:
            print_post_decrement(node);
            break;

        case AST_PARENTHESIS:
            print_parenthesis(node);
            break;

        case AST_IDENTIFIER:
            print_identifier(node);
            break;

        // one case per kind would be seven copies of the same line: a literal
        // is written back as the lexeme it was read from, whichever kind it is
        case AST_INTEGER_LITERAL:
        case AST_FLOAT_LITERAL:
        case AST_STRING_LITERAL:
        case AST_CHAR_LITERAL:
        case AST_SYMBOL_LITERAL:
        case AST_TRUE:
        case AST_FALSE:
            print_literal(node);
            break;

        case AST_TEMPLATE_STRING:
            print_template_string(node);
            break;

        case AST_TEMPLATE_STRING_CHUNK:
            print_template_string_chunk(node);
            break;

        case AST_INTERPOLATION:
            print_interpolation(node);
            break;

        // a node kind with no case here would otherwise vanish from the
        // output without a trace, which is the worst way to find out that one
        // is missing
        default:
            print_string("<no printer for ast kind ");
            print_string(std::to_string((int) kind));
            print_string(">");
            break;
    }
}

void PrettyPrinter::print_module(u32 node) {
    print_children_joined(node, "\n");
}

void PrettyPrinter::print_import(u32 node) {
    print_string("import ");
    print_children(node);
}

void PrettyPrinter::print_import_path(u32 node) {
    print_children_joined(node, ".");
}

void PrettyPrinter::print_import_path_segment(u32 node) {
    print_node_token(node);
}

void PrettyPrinter::print_import_alias(u32 node) {
    print_string(" as ");
    print_node_token(node);
}

void PrettyPrinter::print_let_declaration(u32 node) {
    print_string("let ");
    print_children(node);
}

void PrettyPrinter::print_const_declaration(u32 node) {
    print_string("const ");
    print_children(node);
}

// the header is one line and the parameters and the body are lines of their
// own inside it, so which part goes where is decided by the kind of each child
void PrettyPrinter::print_function(u32 node) {
    print_string("def ");

    u32 child = ast->get_node(node)->get_children();

    while (child != 0) {
        AstNodeKind kind = ast->get_node(child)->get_kind();

        // a parameter needs a line of its own, one level in. The block raises
        // the level itself, so it must not be raised for it here as well
        if (kind == AST_PARAM) {
            ++indentation;
            print_new_line();
            print_node(child);
            --indentation;
        } else {
            print_node(child);
        }

        child = ast->get_node(child)->get_sibling();
    }
}

void PrettyPrinter::print_generic_parameters(u32 node) {
    print_string("<");
    print_children_joined(node, ", ");
    print_string(">");
}

void PrettyPrinter::print_function_return_type(u32 node) {
    print_string(" : ");
    print_children(node);
}

// every statement on a line of its own, one level deeper than the line that
// opened the block. A block with no statements writes nothing, which is what
// makes 'if a:' with an empty body print as itself
void PrettyPrinter::print_block(u32 node) {
    u32 child = ast->get_node(node)->get_children();

    ++indentation;

    while (child != 0) {
        print_new_line();
        print_node(child);
        child = ast->get_node(child)->get_sibling();
    }

    --indentation;
}

void PrettyPrinter::print_pass(u32 node) {
    print_node_token(node);
}

void PrettyPrinter::print_if(u32 node) {
    print_conditional(node, "if ");
}

void PrettyPrinter::print_elif(u32 node) {
    print_conditional(node, "elif ");
}

void PrettyPrinter::print_else(u32 node) {
    print_conditional(node, "else");
}

void PrettyPrinter::print_while(u32 node) {
    print_conditional(node, "while ");
}

void PrettyPrinter::print_return(u32 node) {
    print_jump(node, "return");
}

void PrettyPrinter::print_break(u32 node) {
    print_jump(node, "break");
}

void PrettyPrinter::print_continue(u32 node) {
    print_jump(node, "continue");
}

void PrettyPrinter::print_yield(u32 node) {
    print_jump(node, "yield");
}

void PrettyPrinter::print_goto(u32 node) {
    print_jump(node, "goto");
}

// the space belongs to the expression, not to the keyword: a bare 'return' must
// not be written with a trailing space, or it stops being what was read
void PrettyPrinter::print_jump(u32 node, const std::string& keyword) {
    print_string(keyword);

    if (ast->get_node(node)->get_children() == 0) {
        return;
    }

    print_string(" ");
    print_children(node);
}

// the parts are told apart by kind rather than by position, so a header whose
// condition failed to parse still writes its ':' and its block. The elif and
// the else go back to the level of the header they belong to, which is where
// the indentation already is: the block raised it and lowered it again
void PrettyPrinter::print_conditional(u32 node, const std::string& keyword) {
    print_string(keyword);

    u32 child = ast->get_node(node)->get_children();

    while (child != 0) {
        AstNodeKind kind = ast->get_node(child)->get_kind();

        if (kind == AST_BLOCK) {
            print_string(":");
            print_node(child);
        } else if (kind == AST_ELIF || kind == AST_ELSE) {
            print_new_line();
            print_node(child);
        } else {
            print_node(child);
        }

        child = ast->get_node(child)->get_sibling();
    }
}

void PrettyPrinter::print_param(u32 node) {
    print_string("@");
    print_children(node);
}

void PrettyPrinter::print_binding(u32 node) {
    print_children(node);
}

void PrettyPrinter::print_binding_name(u32 node) {
    print_children(node);
}

void PrettyPrinter::print_binding_type(u32 node) {
    print_string(" : ");
    print_children(node);
}

void PrettyPrinter::print_binding_expression(u32 node) {
    print_string(" = ");
    print_children(node);
}

void PrettyPrinter::print_plus_expression(u32 node) {
    print_children_joined(node, " + ");
}

void PrettyPrinter::print_minus_expression(u32 node) {
    print_children_joined(node, " - ");
}

void PrettyPrinter::print_times_expression(u32 node) {
    print_children_joined(node, " * ");
}

void PrettyPrinter::print_division_expression(u32 node) {
    print_children_joined(node, " / ");
}

void PrettyPrinter::print_integer_division_expression(u32 node) {
    print_children_joined(node, " // ");
}

void PrettyPrinter::print_modulo_expression(u32 node) {
    print_children_joined(node, " % ");
}

// the two forms are told apart by the number of children, because the builder
// writes the '::name' form with no alias: one child is '::name', two are
// 'alias::name'. The '::' is printed glued, the way the '.' of an import path
// is — it resolves a name, it is not an operator between operands
void PrettyPrinter::print_scope(u32 node) {
    u32 child = ast->get_node(node)->get_children();

    if (child != 0 && ast->get_node(child)->get_sibling() == 0) {
        print_string("::");
        print_node(child);
        return;
    }

    print_children_joined(node, "::");
}

void PrettyPrinter::print_assignment_expression(u32 node) {
    print_binary_from_token(node);
}

void PrettyPrinter::print_cast_expression(u32 node) {
    print_binary_from_token(node);
}

void PrettyPrinter::print_logical_expression(u32 node) {
    print_binary_from_token(node);
}

void PrettyPrinter::print_comparison_expression(u32 node) {
    print_binary_from_token(node);
}

// the token is the 'not'; the 'in' after it has none of its own
void PrettyPrinter::print_not_in_expression(u32 node) {
    print_children_joined(node, " not in ");
}

// a range is written against its ends, the way a '..' reads: '0..len', not
// '0 .. len'
void PrettyPrinter::print_range_expression(u32 node) {
    print_children_joined(node, std::string(
        context->get_token_value(ast->get_node(node)->get_token())));
}

// The operator comes from the node's token rather than a literal, because more
// than one lexeme can build the same kind: 'and' and '&&' are one operator, and
// the twelve assignments are one printer. What the source chose is kept in the
// tree and written back unchanged.
void PrettyPrinter::print_binary_from_token(u32 node) {
    std::string oper = " ";

    oper += context->get_token_value(ast->get_node(node)->get_token());
    oper += " ";

    print_children_joined(node, oper);
}

void PrettyPrinter::print_power_expression(u32 node) {
    print_children_joined(node, " ** ");
}

void PrettyPrinter::print_bitwise_or_expression(u32 node) {
    print_children_joined(node, " | ");
}

void PrettyPrinter::print_bitwise_xor_expression(u32 node) {
    print_children_joined(node, " ^ ");
}

void PrettyPrinter::print_bitwise_and_expression(u32 node) {
    print_children_joined(node, " & ");
}

void PrettyPrinter::print_left_shift_expression(u32 node) {
    print_children_joined(node, " << ");
}

void PrettyPrinter::print_right_shift_expression(u32 node) {
    print_children_joined(node, " >> ");
}

void PrettyPrinter::print_unsigned_right_shift_expression(u32 node) {
    print_children_joined(node, " >>> ");
}

// 'not' is a word, so it carries its own separating space; the symbols are
// written against what they apply to
void PrettyPrinter::print_logical_not(u32 node) {
    print_prefix(node, "not ");
}

void PrettyPrinter::print_logical_not_operator(u32 node) {
    print_prefix(node, "!");
}

void PrettyPrinter::print_address_of(u32 node) {
    print_prefix(node, "&");
}

void PrettyPrinter::print_dereference(u32 node) {
    print_prefix(node, "*");
}

void PrettyPrinter::print_bitwise_not(u32 node) {
    print_prefix(node, "~");
}

void PrettyPrinter::print_unary_minus(u32 node) {
    print_prefix(node, "-");
}

void PrettyPrinter::print_unary_plus(u32 node) {
    print_prefix(node, "+");
}

void PrettyPrinter::print_pre_increment(u32 node) {
    print_prefix(node, "++");
}

void PrettyPrinter::print_pre_decrement(u32 node) {
    print_prefix(node, "--");
}

void PrettyPrinter::print_prefix(u32 node, const std::string& oper) {
    print_string(oper);
    print_children(node);
}

// glued, like the '.' of an import path and the '::' of a scope: these reach
// into a name, they are not operators standing between two operands
void PrettyPrinter::print_dot(u32 node) {
    print_children_joined(node, ".");
}

void PrettyPrinter::print_arrow(u32 node) {
    print_children_joined(node, "->");
}

// the target then the subscript, positionally: both are expressions, so unlike
// everywhere else there is no kind to tell them apart
void PrettyPrinter::print_index(u32 node) {
    u32 target = ast->get_node(node)->get_children();

    print_node(target);
    print_string("[");

    if (target != 0) {
        print_node(ast->get_node(target)->get_sibling());
    }

    print_string("]");
}

// the callee then the arguments, and the arguments carry their own parentheses
void PrettyPrinter::print_call(u32 node) {
    print_children(node);
}

void PrettyPrinter::print_arguments(u32 node) {
    print_string("(");
    print_children_joined(node, ", ");
    print_string(")");
}

void PrettyPrinter::print_post_increment(u32 node) {
    print_postfix(node, "++");
}

void PrettyPrinter::print_post_decrement(u32 node) {
    print_postfix(node, "--");
}

void PrettyPrinter::print_postfix(u32 node, const std::string& oper) {
    print_children(node);
    print_string(oper);
}

// the parentheses are a node, so printing them is a walk like everything else:
// the printer never asks what binds tighter than what. The grouping the source
// had is in the tree, and this writes it back
void PrettyPrinter::print_parenthesis(u32 node) {
    print_string("(");
    print_children(node);
    print_string(")");
}

void PrettyPrinter::print_identifier(u32 node) {
    print_node_token(node);
}

void PrettyPrinter::print_literal(u32 node) {
    print_node_token(node);
}

// the node's token is the opening quote, and a template string is closed by the
// same character it was opened with, so one token writes both ends. It has to
// come from the token: the source chose between ' and " and the tree is where
// that choice is kept
void PrettyPrinter::print_template_string(u32 node) {
    print_node_token(node);
    print_children(node);
    print_node_token(node);
}

void PrettyPrinter::print_template_string_chunk(u32 node) {
    print_node_token(node);
}

void PrettyPrinter::print_interpolation(u32 node) {
    print_string("${");
    print_children(node);
    print_string("}");
}

void PrettyPrinter::print_children(u32 node) {
    u32 child = ast->get_node(node)->get_children();

    while (child != 0) {
        print_node(child);
        child = ast->get_node(child)->get_sibling();
    }
}

void PrettyPrinter::print_children_joined(u32 node, const std::string& sep) {
    bool first = true;
    u32 child = ast->get_node(node)->get_children();

    while (child != 0) {
        if (!first) {
            print_string(sep);
        }

        print_node(child);
        child = ast->get_node(child)->get_sibling();
        first = false;
    }
}

void PrettyPrinter::print_node_token(u32 node) {
    print_token(ast->get_node(node)->get_token());
}

void PrettyPrinter::print_token(u32 token) {
    print_string(context->get_token_value(token));
}

// The one thing the printer checks about what it already wrote, and it is not a
// judgement about the tree: '- -a' is a negated negation, and writing it as
// '--a' would read back as a pre-decrement — a different program. So when the
// two would paste into such a token, a space goes between them.
//
// It costs nothing anywhere else: '-1' keeps its digit, '**p' stays '**p' and
// '~-a' stays '~-a', because none of those paste into anything.
void PrettyPrinter::print_string(const std::string_view& s) {
    if (s.empty()) {
        return;
    }

    if (would_paste(last_character, s.front())) {
        output << ' ';
    }

    output << s;
    last_character = s.back();
}

void PrettyPrinter::print_new_line() {
    output << '\n' << std::string(indentation * 4, ' ');
    last_character = ' ';
}


void PrettyPrinter::set_context(Context* context) {
    this->context = context;
    this->ast = context == nullptr ? nullptr : context->get_ast();
}
