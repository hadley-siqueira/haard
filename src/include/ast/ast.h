#ifndef HAARD_AST_H
#define HAARD_AST_H

#include "ast/modules.h"
#include "ast/module.h"
#include "ast/import.h"
#include "ast/function.h"
#include "ast/class.h"
#include "ast/struct.h"

#include "ast/statement.h"
#include "ast/compound_statement.h"
#include "ast/expression_statement.h"
#include "ast/statements/while_statement.h"
#include "ast/statements/for_statement.h"
#include "ast/statements/return_statement.h"

#include "ast/type.h"
#include "ast/subtyped_type.h"
#include "ast/pointer_type.h"
#include "ast/reference_type.h"
#include "ast/list_type.h"
#include "ast/function_type.h"
#include "ast/array_type.h"
#include "ast/tuple_type.h"
#include "ast/named_type.h"

#include "ast/expression.h"

// Binary operators
#include "ast/binary_operators/binary_operator.h"

#include "ast/binary_operators/assignment.h"
#include "ast/binary_operators/bitwise_and_assignment.h"
#include "ast/binary_operators/bitwise_xor_assignment.h"
#include "ast/binary_operators/bitwise_or_assignment.h"
#include "ast/binary_operators/bitwise_not_assignment.h"
#include "ast/binary_operators/division_assignment.h"
#include "ast/binary_operators/integer_division_assignment.h"
#include "ast/binary_operators/minus_assignment.h"
#include "ast/binary_operators/modulo_assignment.h"
#include "ast/binary_operators/plus_assignment.h"
#include "ast/binary_operators/times_assignment.h"
#include "ast/binary_operators/shift_left_logical_assignment.h"
#include "ast/binary_operators/shift_right_arithmetic_assignment.h"
#include "ast/binary_operators/shift_right_logical_assignment.h"

#include "ast/binary_operators/cast.h"

#include "ast/binary_operators/logical_or.h"
#include "ast/binary_operators/logical_and.h"

#include "ast/binary_operators/equal.h"
#include "ast/binary_operators/not_equal.h"

#include "ast/binary_operators/less_than.h"
#include "ast/binary_operators/greater_than.h"
#include "ast/binary_operators/less_than_or_equal.h"
#include "ast/binary_operators/greater_than_or_equal.h"

#include "ast/binary_operators/in.h"
#include "ast/binary_operators/not_in.h"

#include "ast/binary_operators/inclusive_range.h"
#include "ast/binary_operators/exclusive_range.h"

#include "ast/binary_operators/plus.h"
#include "ast/binary_operators/minus.h"

#include "ast/binary_operators/times.h"
#include "ast/binary_operators/division.h"
#include "ast/binary_operators/modulo.h"
#include "ast/binary_operators/integer_division.h"

#include "ast/binary_operators/power.h"

#include "ast/binary_operators/bitwise_or.h"
#include "ast/binary_operators/bitwise_xor.h"
#include "ast/binary_operators/bitwise_and.h"

#include "ast/binary_operators/shift_left_logical.h"
#include "ast/binary_operators/shift_right_arithmetic.h"
#include "ast/binary_operators/shift_right_logical.h"

#include "ast/binary_operators/dot.h"
#include "ast/binary_operators/arrow.h"
#include "ast/binary_operators/index.h"
#include "ast/binary_operators/call.h"
#include "ast/binary_operators/hash_pair.h"

#include "ast/unary_operators/unary_operator.h"
#include "ast/unary_operators/address_of.h"
#include "ast/unary_operators/bitwise_not.h"
#include "ast/unary_operators/delete_array.h"
#include "ast/unary_operators/delete.h"
#include "ast/unary_operators/dereference.h"
#include "ast/unary_operators/logical_not.h"
#include "ast/unary_operators/new.h"
#include "ast/unary_operators/pos_decrement.h"
#include "ast/unary_operators/pos_increment.h"
#include "ast/unary_operators/pre_decrement.h"
#include "ast/unary_operators/pre_increment.h"
#include "ast/unary_operators/sizeof.h"
#include "ast/unary_operators/unary_minus.h"
#include "ast/unary_operators/unary_operator.h"
#include "ast/unary_operators/unary_plus.h"
#include "ast/unary_operators/parenthesis.h"

#include "ast/expression_list.h"
#include "ast/identifier.h"

#include "ast/literals/literal.h"
#include "ast/literals/boolean_literal.h"
#include "ast/literals/integer_literal.h"
#include "ast/literals/float_literal.h"
#include "ast/literals/double_literal.h"
#include "ast/literals/char_literal.h"
#include "ast/literals/string_literal.h"
#include "ast/literals/list_literal.h"
#include "ast/literals/array_literal.h"
#include "ast/literals/hash_literal.h"
#include "ast/literals/tuple_literal.h"

#include "ast/sequence.h"

#endif
