// The pretty printer walks an ast, and nothing builds one yet: parser.cpp is a
// half written refactor. So the ast here is built by hand, node by node, the
// way the parser is meant to build it, and what is checked is the text that
// comes out.
//
// The trees are built through AstBuilder, the same way the parser will build
// them, so these tests cover the builder as much as the printer.
//
// The tokens are real: cases/declarations.hd is scanned first, and the nodes
// point at the tokens of that file by index. The comment above each test lists
// the ones it uses.
//
// No framework: one binary, one line per check, non-zero exit when any failed.
#include <haard/ast/ast_builder.h>
#include <haard/context/context.h>
#include <haard/pretty_printer/pretty_printer.h>
#include <haard/scanner/scanner.h>
#include <iostream>
#include <sstream>

using namespace haard;

int failures = 0;

std::string escape(const std::string& s) {
    std::string r;

    for (char c : s) {
        if (c == '\n') {
            r += "\\n";
        } else {
            r += c;
        }
    }

    return r;
}

void check(const std::string& name, const std::string& got,
           const std::string& want) {
    if (got == want) {
        std::cout << "PASS  " << name << "\n";
        return;
    }

    ++failures;
    std::cout << "FAIL  " << name << "\n"
              << "       got: '" << escape(got) << "'\n"
              << "      want: '" << escape(want) << "'\n";
}

std::string print(Context& context) {
    PrettyPrinter printer;
    std::stringstream out;

    printer.set_context(&context);

    if (!printer.print(out)) {
        return "<nothing to print>";
    }

    return out.str();
}

// import std.io as io
//  0      1 2 3  4  5
void test_import(Context& context, AstBuilder& builder) {
    u32 path = builder.make_import_path();
    u32 last = builder.add_child(path, 0, builder.make_import_path_segment(1));

    builder.add_child(path, last, builder.make_import_path_segment(3));

    u32 import = builder.make_import(0, path, builder.make_import_alias(5));
    u32 module = builder.make_module();

    builder.add_child(module, 0, import);

    check("import with a path and an alias", print(context),
          "import std.io as io\n");
}

// let x : i32 = a + b
//  6  7 8  9  10 11 12 13
void test_let_declaration(Context& context, AstBuilder& builder) {
    u32 name = builder.make_binding_name(builder.make_identifier(7));
    u32 type = builder.make_binding_type(builder.make_identifier(9));

    u32 sum = builder.make_binary_operator(AST_PLUS, 12,
                                           builder.make_identifier(11),
                                           builder.make_identifier(13));

    u32 binding = builder.make_binding(name, type,
                                       builder.make_binding_expression(sum));
    u32 module = builder.make_module();

    builder.add_child(module, 0, builder.make_let_declaration(6, binding));

    check("let declaration with a type and an expression", print(context),
          "let x : i32 = a + b\n");
}

// @count : u32
//  14 15 16 17
void test_param(Context& context, AstBuilder& builder) {
    u32 name = builder.make_binding_name(builder.make_identifier(15));
    u32 type = builder.make_binding_type(builder.make_identifier(17));

    u32 module = builder.make_module();

    builder.add_child(module, 0, builder.make_param(14, name, type));

    check("param", print(context), "@count : u32\n");
}

// let p = io::println
//  18 19 20 21 22 23
void test_scope_with_an_alias(Context& context, AstBuilder& builder) {
    u32 name = builder.make_binding_name(builder.make_identifier(19));

    u32 scope = builder.make_scope(22, builder.make_identifier(21),
                                   builder.make_identifier(23));

    u32 binding = builder.make_binding(name, 0,
                                       builder.make_binding_expression(scope));
    u32 module = builder.make_module();

    builder.add_child(module, 0, builder.make_let_declaration(18, binding));

    check("scope with an alias", print(context), "let p = io::println\n");
}

// let q = ::println
//  24 25 26 27 28
//
// the '::name' form has no alias, so it is written with a 0 and the node is
// left with a single child. That is what the printer reads to tell the two
// forms apart
void test_scope_without_an_alias(Context& context, AstBuilder& builder) {
    u32 name = builder.make_binding_name(builder.make_identifier(25));
    u32 scope = builder.make_scope(27, 0, builder.make_identifier(28));

    u32 binding = builder.make_binding(name, 0,
                                       builder.make_binding_expression(scope));
    u32 module = builder.make_module();

    builder.add_child(module, 0, builder.make_let_declaration(24, binding));

    check("scope with no alias", print(context), "let q = ::println\n");
}

// let s = (a + b) * c
//  41 42 43 44 45 46 47 48 49 50
//
// the parentheses are a node, so the printer writes them by walking the tree
// and never has to work out where they would be needed. The tree here is the
// one the parser builds for that line
void test_parenthesis(Context& context, AstBuilder& builder) {
    u32 sum = builder.make_binary_operator(AST_PLUS, 46,
                                           builder.make_identifier(45),
                                           builder.make_identifier(47));

    u32 group = builder.make_parenthesis(44, sum);
    u32 module = builder.make_module();

    builder.add_child(module, 0,
        builder.make_binary_operator(AST_TIMES, 49, group,
                                     builder.make_identifier(50)));

    check("a parenthesised operand", print(context), "(a + b) * c\n");
}

// the same operators with no parentheses in the source have none in the tree,
// so nothing is added on the way out: the printer is a walk, not a decision
void test_no_parenthesis_is_added(Context& context, AstBuilder& builder) {
    u32 product = builder.make_binary_operator(AST_TIMES, 49,
                                               builder.make_identifier(45),
                                               builder.make_identifier(47));

    u32 module = builder.make_module();

    builder.add_child(module, 0,
        builder.make_binary_operator(AST_PLUS, 46,
                                     builder.make_identifier(50), product));

    check("nothing is parenthesised on its own", print(context),
          "c + a * b\n");
}

// a division and a modulo, so every operator of the level has a printer
void test_division_and_modulo(Context& context, AstBuilder& builder) {
    u32 module = builder.make_module();

    u32 last = builder.add_child(module, 0,
        builder.make_binary_operator(AST_DIVISION, 35,
                                     builder.make_identifier(32),
                                     builder.make_identifier(34)));

    builder.add_child(module, last,
        builder.make_binary_operator(AST_MODULO, 37,
                                     builder.make_identifier(34),
                                     builder.make_identifier(36)));

    check("division and modulo", print(context), "a / b\nb % c\n");
}

// a part that is not there is a 0, and needs no special case: 'let x' is a
// binding with no type and no expression
void test_binding_without_type(Context& context, AstBuilder& builder) {
    u32 name = builder.make_binding_name(builder.make_identifier(7));
    u32 binding = builder.make_binding(name, 0, 0);
    u32 module = builder.make_module();

    builder.add_child(module, 0, builder.make_let_declaration(6, binding));

    check("a binding with no type and no expression", print(context),
          "let x\n");
}

// declarations in a row: the module joins them with a line break. Three of
// them, not two: with two, appending to the first child and appending to the
// last child give the same tree
void test_module_with_three_children(Context& context, AstBuilder& builder) {
    u32 module = builder.make_module();
    u32 last = 0;

    last = builder.add_child(module, last, builder.make_let_declaration(6,
        builder.make_binding(builder.make_binding_name(
            builder.make_identifier(7)), 0, 0)));

    last = builder.add_child(module, last, builder.make_const_declaration(6,
        builder.make_binding(builder.make_binding_name(
            builder.make_identifier(15)), 0, 0)));

    builder.add_child(module, last, builder.make_let_declaration(6,
        builder.make_binding(builder.make_binding_name(
            builder.make_identifier(11)), 0, 0)));

    check("module joins its children with a line break", print(context),
          "let x\nconst count\nlet a\n");
}

// a node kind the switch has no case for must be visible in the output rather
// than disappearing from it. The builder has no way to make one, so the test
// pushes it straight into the container
void test_unknown_kind(Context& context, AstBuilder& builder) {
    AstNode node;
    u32 module = builder.make_module();

    node.set_kind(AST_UNKNOWN);
    builder.add_child(module, 0, context.get_ast()->push(node));

    check("a kind with no printer is reported in the output", print(context),
          "<no printer for ast kind 0>\n");
}

// the fields of a node as one line, to check a whole node at once
std::string state_of(Ast* ast, u32 index) {
    AstNode* node = ast->get_node(index);

    return "kind=" + std::to_string((int) node->get_kind())
         + " token=" + std::to_string(node->get_token())
         + " sibling=" + std::to_string(node->get_sibling())
         + " children=" + std::to_string(node->get_children());
}

// index 0 is the sentinel: it is what 'there is no node here' points at, and a
// walk that reaches it stops because its sibling and its children are 0.
// Building a whole tree must leave it exactly as it was created
void test_sentinel_is_untouched(Context& context, AstBuilder& builder) {
    u32 module = builder.make_module();

    builder.add_child(module, 0, builder.make_let_declaration(6,
        builder.make_binding(builder.make_binding_name(
            builder.make_identifier(7)), 0, 0)));

    check("building a tree does not touch the sentinel",
          state_of(context.get_ast(), 0),
          "kind=0 token=0 sibling=0 children=0");
}

// an index that is not a node gives back the sentinel, so a walk over a broken
// tree stops instead of reading past the vector
void test_out_of_range_is_the_sentinel(Context& context, AstBuilder& builder) {
    u32 module = builder.make_module();

    builder.add_child(module, 0, builder.make_identifier(7));

    check("an index past the end gives back the sentinel",
          state_of(context.get_ast(), 9999),
          "kind=0 token=0 sibling=0 children=0");
}

// A list node with as many children as a big array literal would have. This is
// a performance test as much as a correctness one: an append that has to look
// the end of the list up costs one walk per child, so this takes forty seconds
// that way and the runner's timeout turns it into a failure. Carrying the last
// child along, it takes under a millisecond.
void test_long_list_of_children(Context& context, AstBuilder& builder) {
    Ast* ast = context.get_ast();
    u32 total = 100000;
    u32 module = builder.make_module();
    u32 last = 0;
    u32 first = 0;

    for (u32 i = 0; i < total; ++i) {
        last = builder.add_child(module, last, builder.make_identifier(7));

        if (first == 0) {
            first = last;
        }
    }

    u32 walked = 0;
    u32 child = ast->get_node(module)->get_children();
    bool in_order = child == first;

    while (child != 0) {
        ++walked;
        child = ast->get_node(child)->get_sibling();
    }

    check("a long list keeps every child, in order",
          std::to_string(walked) + (in_order ? " in order" : " out of order"),
          std::to_string(total) + " in order");
}

// an ast nobody built yet is not an error, it is nothing to print
void test_empty_ast() {
    Context context;

    check("an empty ast prints nothing", print(context), "<nothing to print>");
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "usage: test_pretty_printer <declarations.hd>\n";
        return 2;
    }

    // one test per Context: the ast is append only and each test builds a
    // module of its own, so they cannot share one
    for (auto test : { test_import, test_let_declaration, test_param,
                       test_scope_with_an_alias, test_scope_without_an_alias,
                       test_parenthesis, test_no_parenthesis_is_added,
                       test_division_and_modulo,
                       test_binding_without_type, test_module_with_three_children,
                       test_unknown_kind, test_sentinel_is_untouched,
                       test_out_of_range_is_the_sentinel,
                       test_long_list_of_children }) {
        Context context;
        Scanner scanner;
        AstBuilder builder;

        scanner.set_context(&context);
        builder.set_ast(context.get_ast());
        scanner.get_tokens(argv[1]);

        if (context.get_logger()->has_errors()) {
            std::cout << "FAIL  the case file did not scan cleanly\n";
            context.get_logger()->print(std::cout);
            return 1;
        }

        test(context, builder);
    }

    test_empty_ast();

    return failures > 0 ? 1 : 0;
}
