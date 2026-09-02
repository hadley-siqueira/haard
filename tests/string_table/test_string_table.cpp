// The interned names of one module, and the hash record 0014 pins.
//
// Two things are worth reading before changing anything here. The hash values
// below are not what this implementation happens to produce: they are FNV-1a's
// published test vectors, so they fail if the constants drift or if the xor
// and the multiply swap places -- that second one is FNV-1, a different
// function that record 0014 deliberately did not choose.
//
// And the collisions are real ones. "costarring"/"liquid" and
// "declinate"/"macallums" are documented FNV-1a 32-bit collisions, which is
// what lets these tests prove that the byte comparison is load-bearing instead
// of asserting it.
//
// No framework: one binary, one line per check, non-zero exit when any failed.
#include <haard/string_table/string_table.h>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <string>

using namespace haard;

int failures = 0;

void check(const std::string& name, const std::string& got,
           const std::string& want) {
    if (got == want) {
        std::cout << "PASS  " << name << "\n";
        return;
    }

    ++failures;
    std::cout << "FAIL  " << name << "\n"
              << "       got: '" << got << "'\n"
              << "      want: '" << want << "'\n";
}

void check(const std::string& name, u32 got, u32 want) {
    check(name, std::to_string(got), std::to_string(want));
}

std::string hex(u32 value) {
    std::ostringstream out;

    out << "0x" << std::hex << std::setw(8) << std::setfill('0') << value;

    return out.str();
}

// FNV-1a's own test vectors. The empty string is the offset basis untouched,
// which is the one case that proves the basis alone
void the_hash_is_fnv1a() {
    check("the empty string hashes to the offset basis",
          hex(hash_name("")), "0x811c9dc5");
    check("'a' hashes to fnv1a's vector", hex(hash_name("a")), "0xe40c292c");
    check("'foobar' hashes to fnv1a's vector",
          hex(hash_name("foobar")), "0xbf9cf968");

    // FNV-1 gives 0x050c5d7e for "a": if this ever equals that, the xor and
    // the multiply have swapped places
    check("it is fnv1a and not fnv1", hash_name("a") != 0x050c5d7e ? 1 : 0, 1);
}

void a_name_is_stored_once() {
    StringTable table;

    u32 first = table.intern("println");
    u32 again = table.intern("println");
    u32 other = table.intern("print");

    check("interning the same name twice gives one index", first, again);
    check("a different name gets a different index", other != first ? 1 : 0, 1);
    check("and the table holds two names", table.get_count(), 2);
}

void an_entry_keeps_its_text_and_its_hash() {
    StringTable table;

    u32 index = table.intern("compress");

    check("the text comes back", std::string(table.get_text(index)),
          "compress");
    check("and the hash is the one hash_name gives",
          hex(table.get_hash(index)), hex(hash_name("compress")));
}

void the_empty_name_is_a_name() {
    StringTable table;

    u32 index = table.intern("");

    check("the empty string interns", table.get_count(), 1);
    check("and comes back empty", std::string(table.get_text(index)), "");
}

// The cross-module probe of record 0013: module A holds the hash and the
// bytes, and asks module B's table whether it knows the name. A's own index
// means nothing here, which is why find takes a hash and not an index
void a_name_is_found_by_a_hash_computed_elsewhere() {
    StringTable module_a;
    StringTable module_b;

    module_a.intern("archive");
    u32 in_b = module_b.intern("archive");

    u32 hash = module_a.get_hash(module_a.find(hash_name("archive"),
                                               "archive"));

    check("a hash from another table finds the name",
          module_b.find(hash, "archive"), in_b);
    check("and a name the table does not have is not found",
          module_b.find(hash_name("compress"), "compress"), INVALID_STRING);
}

// Two real FNV-1a collisions. Without the byte comparison in find(), the
// second name of each pair would come back as the first: one index for two
// different names, and every use of one resolving to the other
void two_names_of_one_hash_stay_two_names() {
    StringTable table;

    check("costarring and liquid really do collide",
          hex(hash_name("costarring")), hex(hash_name("liquid")));

    u32 costarring = table.intern("costarring");
    u32 liquid = table.intern("liquid");

    check("colliding names get different indices",
          costarring != liquid ? 1 : 0, 1);
    check("and the table holds both", table.get_count(), 2);
    check("the first keeps its text", std::string(table.get_text(costarring)),
          "costarring");
    check("the second keeps its text", std::string(table.get_text(liquid)),
          "liquid");

    check("find picks the one whose bytes match",
          table.find(hash_name("liquid"), "liquid"), liquid);
    check("and the other one by its bytes",
          table.find(hash_name("costarring"), "costarring"), costarring);

    // a name that is in neither, but hashes into the same bucket
    check("a name sharing the hash and absent is not found",
          table.find(hash_name("costarring"), "macallums"), INVALID_STRING);
}

// interning one of a colliding pair twice must not append: the bucket is
// walked, the bytes match the first entry, and nothing new is stored
void a_collision_does_not_break_deduplication() {
    StringTable table;

    table.intern("declinate");
    table.intern("macallums");
    table.intern("declinate");
    table.intern("macallums");

    check("two colliding names interned twice are still two",
          table.get_count(), 2);
}

int main() {
    std::cout << "\n";

    the_hash_is_fnv1a();
    a_name_is_stored_once();
    an_entry_keeps_its_text_and_its_hash();
    the_empty_name_is_a_name();
    a_name_is_found_by_a_hash_computed_elsewhere();
    two_names_of_one_hash_stay_two_names();
    a_collision_does_not_break_deduplication();

    return failures > 0 ? 1 : 0;
}
