#include <stdio.h>
#include <string.h>
#include "../crengine/include/crengine.h"
#include "../crengine/include/lvstring8collection.h"
#include "../crengine/include/lvstring32collection.h"
#include "stringtest.h"

static int test_errors = 0;
#define TCHECK(cond) do { if (!(cond)) { printf("FAIL line %d: %s\n", __LINE__, #cond); test_errors++; } } while(0)

#define CHECKBUF_(file, line) check_ls_storage(file ":" #line)
#define CHECKBUF CHECKBUF_(__FILE__, __LINE__)

void testStrings8_16() {
    printf("=== MIX lString8 tests ===\n");

           // Constructors
    lString8 s8_default;
    TCHECK(s8_default.empty());

    lString8 s8_size(10);
    TCHECK(s8_size.empty());

    lString8 s8_cstr("hello");
    TCHECK(s8_cstr.length() == 5);
    TCHECK(s8_cstr == "hello");

    lString8 s8_fragment("hello world", 5);
    TCHECK(s8_fragment == "hello");

    lString8 s8_sub(s8_cstr, 1, 3);
    TCHECK(s8_sub == "ell");

           // Copy / move
    lString8 s8_copy(s8_cstr);
    TCHECK(s8_copy == "hello");
    lString8 s8_move(std::move(s8_copy));
    TCHECK(s8_move == "hello");

           // Assignment
    lString8 s8_assign;
    s8_assign = s8_move;
    TCHECK(s8_assign == "hello");
    lString8 s8_assign2;
    s8_assign2 = "world";
    TCHECK(s8_assign2 == "world");
    lString8 s8_assign3;
    s8_assign3 = lString8("temp");
    TCHECK(s8_assign3 == "temp");

           // assign methods
    lString8 s8_a;
    s8_a.assign("test");
    TCHECK(s8_a == "test");
    s8_a.assign("abcdef", 3);
    TCHECK(s8_a == "abc");
    lString8 s8_b("xyz");
    s8_a.assign(s8_b);
    TCHECK(s8_a == "xyz");
    s8_a.assign(lString8("move_assign"));
    TCHECK(s8_a == "move_assign");

    CHECKBUF;

           // Append
    lString8 s8_app;
    s8_app.append("foo");
    TCHECK(s8_app == "foo");
    s8_app.append("xy", 1);
    TCHECK(s8_app == "foox");
    s8_app.append(3, '!');
    TCHECK(s8_app == "foox!!!");
    s8_app.clear();

           // appendDecimal / appendHex
    s8_app.appendDecimal(12345);
    TCHECK(s8_app == "12345");
    s8_app.clear();
    s8_app.appendDecimal(-42);
    TCHECK(s8_app == "-42");
    s8_app.clear();
    s8_app.appendHex(0xFF);
    TCHECK(s8_app == "ff");
    s8_app.clear();

           // Insert
    lString8 s8_ins("world");
    s8_ins.insert(0, 2, 'X');
    TCHECK(s8_ins == "XXworld");

           // Replace
    lString8 s8_rep("hello world");
    s8_rep.replace(0, 5, lString8("good"));
    TCHECK(s8_rep == "good world");
    lString8 s8_rep2("abcabc");
    s8_rep2.replace('a', 'X');
    TCHECK(s8_rep2 == "XbcXbc");

           // Erase
    lString8 s8_era("0123456789");
    s8_era.erase(5, 5);
    TCHECK(s8_era == "01234");

           // uppercase / lowercase
    lString8 s8_case("Hello World");
    s8_case.uppercase();
    TCHECK(s8_case == "HELLO WORLD");
    s8_case.lowercase();
    TCHECK(s8_case == "hello world");

    CHECKBUF;

           // compare
    TCHECK(s8_cstr.compare("hello") == 0);
    TCHECK(s8_cstr.compare("world") < 0);
    TCHECK(s8_cstr.compare("abc") > 0);

           // pos / rpos
    lString8 s8_pos("hello world, hello");
    TCHECK(s8_pos.pos('w') == 6);
    TCHECK(s8_pos.pos("world") == 6);
    TCHECK(s8_pos.pos('h', 1) == 13);
    TCHECK(s8_pos.rpos("hello") == 13);
    TCHECK(s8_pos.pos(lString8("hello"), 1) == 13);

           // startsWith / endsWith
    lString8 s8_sw("hello.cpp");
    TCHECK(s8_sw.startsWith("hello"));
    TCHECK(s8_sw.startsWith(lString8("hello")));
    TCHECK(!s8_sw.startsWith("world"));
    TCHECK(s8_sw.endsWith(".cpp"));

           // substr
    lString8 s8_sub2("hello world");
    TCHECK(s8_sub2.substr(6) == "world");
    TCHECK(s8_sub2.substr(0, 5) == "hello");

           // operator[]
    TCHECK(s8_sub2[0] == 'h');
    TCHECK(s8_sub2[4] == 'o');

           // data / c_str
    TCHECK(strcmp(s8_sub2.c_str(), "hello world") == 0);
    TCHECK(strcmp(s8_sub2.data(), "hello world") == 0);

           // length / size / capacity / empty
    TCHECK(s8_sub2.length() == 11);
    TCHECK(s8_sub2.size() == 11);
    TCHECK(!s8_sub2.empty());
    lString8 s8_empty;
    TCHECK(s8_empty.empty());
    TCHECK(!s8_empty);

           // at
    TCHECK(s8_sub2.at(0) == 'h');
    TCHECK(s8_sub2.at(10) == 'd');

    CHECKBUF;

           // atoi
    lString8 s8_int("12345");
    TCHECK(s8_int.atoi() == 12345);
    lString8 s8_int2("-999");
    TCHECK(s8_int2.atoi() == -999);
    lString8 s8_int64("1234567890123");
    TCHECK(s8_int64.atoi64() == 1234567890123LL);

           // swap
    lString8 s8_swap1("first");
    lString8 s8_swap2("second");
    s8_swap1.swap(s8_swap2);
    TCHECK(s8_swap1 == "second");
    TCHECK(s8_swap2 == "first");

           // trim
    lString8 s8_trim("  spaced  ");
    s8_trim.trim();
    TCHECK(s8_trim == "spaced");

           // pack
    lString8 s8_pack("hello");
    s8_pack.reserve(1000);
    s8_pack.pack();

           // clear / reset
    lString8 s8_clr("something");
    s8_clr.clear();
    TCHECK(s8_clr.empty());
    lString8 s8_rst("something");
    s8_rst.reset(50);
    TCHECK(s8_rst.empty());
    TCHECK(s8_rst.capacity() >= 49);

           // resize
    lString8 s8_rsz("hi");
    s8_rsz.resize(5, 'x');

           // lastChar / firstChar
    lString8 s8_lf("abc");
    TCHECK(s8_lf.firstChar() == 'a');
    TCHECK(s8_lf.lastChar() == 'c');

           // getHash
    lString8 s8_h1("hello");
    lString8 s8_h2("hello");
    TCHECK(s8_h1.getHash() == s8_h2.getHash());

    CHECKBUF;

           // operator <<
    lString8 s8_shift;
    s8_shift << 'A';
    TCHECK(s8_shift == "A");
    s8_shift << "BC";
    TCHECK(s8_shift == "ABC");
    s8_shift << lString8("DE");
    TCHECK(s8_shift == "ABCDE");
    s8_shift.clear();
    s8_shift << fmt::decimal(42);
    TCHECK(s8_shift == "42");
    s8_shift.clear();
    s8_shift << fmt::hex(255);
    TCHECK(s8_shift == "ff");

           // operator +=
    lString8 s8_plus;
    s8_plus += "hello";
    TCHECK(s8_plus == "hello");
    s8_plus += lString8(" world");
    TCHECK(s8_plus == "hello world");
    s8_plus += '!';
    TCHECK(s8_plus == "hello world!");
    s8_plus += fmt::decimal(99);
    TCHECK(s8_plus == "hello world!99");

           // external operator == / !=
    lString8 s8_eq("equal");
    lString8 s8_eq2("equal");
    lString8 s8_neq("not_equal");
    TCHECK(s8_eq == s8_eq2);
    TCHECK(!(s8_eq != s8_eq2));
    TCHECK(s8_eq != s8_neq);
    TCHECK(s8_eq == "equal");
    TCHECK("equal" == s8_eq);
    TCHECK(s8_eq != "other");
    TCHECK("other" != s8_eq);

           // external operator +
    lString8 s8_add("abc");
    lString8 s8_added = s8_add + "def";
    TCHECK(s8_added == "abcdef");
    s8_added = s8_add + lString8("ghi");
    TCHECK(s8_added == "abcghi");
    s8_added = s8_add + fmt::decimal(123);
    TCHECK(s8_added == "abc123");
    s8_added = s8_add + fmt::hex(0xFF);
    TCHECK(s8_added == "abcff");

           // itoa static
    lString8 s8_i = lString8::itoa(42);
    TCHECK(s8_i == "42");
    s8_i = lString8::itoa(0U);
    TCHECK(s8_i == "0");
    s8_i = lString8::itoa(-123LL);
    TCHECK(s8_i == "-123");

           // cs8
    const lString8& cs8_test = cs8("static_string");
    TCHECK(cs8_test == "static_string");
    const lString8& cs8_test2 = cs8("static_string");
    TCHECK(&cs8_test == &cs8_test2);

    CHECKBUF;

    printf("=== MIX lString16 tests ===\n");

           // lString16 constructors
    lString16 s16_default;
    TCHECK(s16_default.empty());
    lString16 s16_cstr(u"hello");
    TCHECK(s16_cstr.length() == 5);
    lString16 s16_from8("hello");
    TCHECK(s16_from8.length() == 5);
    lString16 s16_frag(u"hello world", 5);
    TCHECK(s16_frag == u"hello");
    lString16 s16_sub(s16_cstr, 1, 3);
    TCHECK(s16_sub == u"ell");

           // Copy / move
    lString16 s16_copy(s16_cstr);
    TCHECK(s16_copy == u"hello");
    lString16 s16_move(std::move(s16_copy));
    TCHECK(s16_move == u"hello");

           // Assignment
    lString16 s16_assign;
    s16_assign = u"world";
    TCHECK(s16_assign == u"world");
    s16_assign = "ascii";
    TCHECK(s16_assign == u"ascii");

           // Append
    lString16 s16_app;
    s16_app.append(u"foo");
    TCHECK(s16_app == u"foo");
    s16_app.append((const lChar8*)"bar");
    TCHECK(s16_app == u"foobar");
    s16_app.append(3, u'!');
    TCHECK(s16_app == u"foobar!!!");
    s16_app.clear();

           // appendDecimal / appendHex
    s16_app.appendDecimal(999);
    TCHECK(s16_app == u"999");
    s16_app.clear();
    s16_app.appendHex(0xABC);
    TCHECK(s16_app == u"abc");

           // Insert
    lString16 s16_ins(u"world");
    s16_ins.insert(0, 2, u'X');
    s16_ins.clear();
    s16_ins = u"world";
    s16_ins.insert(0, lString16(u"hello "));

           // compare
    TCHECK(s16_cstr.compare(u"hello") == 0);
    TCHECK(s16_cstr.compare((const lChar8*)"hello") == 0);
    TCHECK(s16_cstr.compare(u"world") < 0);

           // substr
    lString16 s16_sub3(u"hello world");
    TCHECK(s16_sub3.substr(6) == u"world");
    TCHECK(s16_sub3.substr(0, 5) == u"hello");

           // startsWith / endsWith
    TCHECK(s16_sub3.startsWith(u"hello"));
    TCHECK(s16_sub3.startsWith((const lChar8*)"hello"));
    TCHECK(s16_sub3.endsWith(u"world"));
    TCHECK(s16_sub3.endsWith((const lChar8*)"world"));
    TCHECK(s16_sub3.endsWith(lString16(u"world")));

           // operator[]
    TCHECK(s16_sub3[0] == u'h');

           // atoi
    lString16 s16_int(u"6789");
    int s16_ival = 0;
    lInt64 s16_ival64 = 0;
    TCHECK(s16_int.atoi() == 6789);
    TCHECK(s16_int.atoi(s16_ival) && s16_ival == 6789);
    TCHECK(s16_int.atoi(s16_ival64) && s16_ival64 == 6789);

           // getHash
    lString16 s16_h1(u"test");
    lString16 s16_h2(u"test");
    TCHECK(s16_h1.getHash() == s16_h2.getHash());

           // trim / trimNonAlpha
    lString16 s16_tr(u"  spaced  ");
    s16_tr.trim();
    TCHECK(s16_tr == u"spaced");
    lString16 s16_tna(u"!!!hello!!!");
    s16_tna.trimNonAlpha();
    TCHECK(s16_tna == u"hello");

           // swap
    lString16 s16_sw1(u"first");
    lString16 s16_sw2(u"second");
    s16_sw1.swap(s16_sw2);
    TCHECK(s16_sw1 == u"second");
    TCHECK(s16_sw2 == u"first");

           // clear / reset
    lString16 s16_clr(u"something");
    s16_clr.clear();
    TCHECK(s16_clr.empty());
    lString16 s16_rst2(u"something");
    s16_rst2.reset(50);
    TCHECK(s16_rst2.empty());

           // limit
    lString16 s16_lim(u"hello world");
    s16_lim.limit(5);
    TCHECK(s16_lim == u"hello");

           // pack
    lString16 s16_pk(u"test");
    s16_pk.reserve(500);

           // operator <<
    lString16 s16_sh;
    s16_sh << u'A';
    TCHECK(s16_sh == u"A");
    s16_sh << u"BC";
    TCHECK(s16_sh == u"ABC");
    s16_sh << fmt::decimal(42);
    TCHECK(s16_sh == u"ABC42");
    s16_sh.clear();
    s16_sh << fmt::hex(255);
    TCHECK(s16_sh == u"ff");

           // operator +=
    lString16 s16_pe;
    s16_pe += u"hello";
    TCHECK(s16_pe == u"hello");
    s16_pe += lString16(u" world");
    TCHECK(s16_pe == u"hello world");

           // itoa
    lString16 s16_i16 = lString16::itoa(42);
    TCHECK(s16_i16 == u"42");
    s16_i16 = lString16::itoa(0U);
    TCHECK(s16_i16 == u"0");

           // external operator == / != / +
    lString16 s16_eq1(u"equal");
    lString16 s16_eq2(u"equal");
    TCHECK(s16_eq1 == s16_eq2);
    TCHECK(s16_eq1 == u"equal");
    TCHECK(u"equal" == s16_eq1);
    TCHECK(s16_eq1 != u"other");
    lString16 s16_added16 = s16_eq1 + u"_suffix";
    TCHECK(s16_added16 == u"equal_suffix");

    CHECKBUF;
}

void testStrings8() {

    printf("=== lString8 tests ===\n");

           // Constructors
    lString8 s8_default;
    TCHECK(s8_default.empty());

    lString8 s8_size(10);
    TCHECK(s8_size.empty());

    lString8 s8_cstr("hello");
    TCHECK(s8_cstr.length() == 5);
    TCHECK(s8_cstr == "hello");

    lString8 s8_fragment("hello world", 5);
    TCHECK(s8_fragment == "hello");

    lString8 s8_sub(s8_cstr, 1, 3);
    TCHECK(s8_sub == "ell");

    // Copy / move
    lString8 s8_copy(s8_cstr);
    TCHECK(s8_copy == "hello");
    lString8 s8_move(std::move(s8_copy));
    TCHECK(s8_move == "hello");

           // Assignment
    lString8 s8_assign;
    s8_assign = s8_move;
    TCHECK(s8_assign == "hello");
    lString8 s8_assign2;
    s8_assign2 = "world";
    TCHECK(s8_assign2 == "world");
    lString8 s8_assign3;
    s8_assign3 = lString8("temp");
    TCHECK(s8_assign3 == "temp");

    // assign methods
    lString8 s8_a;
    s8_a.assign("test");
    TCHECK(s8_a == "test");
    s8_a.assign("abcdef", 3);
    TCHECK(s8_a == "abc");
    lString8 s8_b("xyz");
    s8_a.assign(s8_b);
    TCHECK(s8_a == "xyz");
    s8_a.assign(lString8("move_assign"));
    TCHECK(s8_a == "move_assign");

           // Append
    lString8 s8_app;
    s8_app.append("foo");
    TCHECK(s8_app == "foo");
    s8_app.append("xy", 1);
    TCHECK(s8_app == "foox");
    s8_app.append(3, '!');
    TCHECK(s8_app == "foox!!!");
    s8_app.clear();

    // appendDecimal / appendHex
    s8_app.appendDecimal(12345);
    TCHECK(s8_app == "12345");
    s8_app.clear();
    s8_app.appendDecimal(-42);
    TCHECK(s8_app == "-42");
    s8_app.clear();
    s8_app.appendHex(0xFF);
    TCHECK(s8_app == "ff");
    s8_app.clear();

    // Insert
    lString8 s8_ins("world");
    s8_ins.insert(0, 2, 'X');
    TCHECK(s8_ins == "XXworld");

           // Replace
    lString8 s8_rep("hello world");
    s8_rep.replace(0, 5, lString8("good"));
    TCHECK(s8_rep == "good world");
    lString8 s8_rep2("abcabc");
    s8_rep2.replace('a', 'X');
    TCHECK(s8_rep2 == "XbcXbc");

           // Erase
    lString8 s8_era("0123456789");
    s8_era.erase(5, 5);
    TCHECK(s8_era == "01234");

           // uppercase / lowercase
    lString8 s8_case("Hello World");
    s8_case.uppercase();
    TCHECK(s8_case == "HELLO WORLD");
    s8_case.lowercase();
    TCHECK(s8_case == "hello world");

           // compare
    TCHECK(s8_cstr.compare("hello") == 0);
    TCHECK(s8_cstr.compare("world") < 0);
    TCHECK(s8_cstr.compare("abc") > 0);

           // pos / rpos
    lString8 s8_pos("hello world, hello");
    TCHECK(s8_pos.pos('w') == 6);
    TCHECK(s8_pos.pos("world") == 6);
    TCHECK(s8_pos.pos('h', 1) == 13);
    TCHECK(s8_pos.rpos("hello") == 13);
    TCHECK(s8_pos.pos(lString8("hello"), 1) == 13);

           // startsWith / endsWith
    lString8 s8_sw("hello.cpp");
    TCHECK(s8_sw.startsWith("hello"));
    TCHECK(s8_sw.startsWith(lString8("hello")));
    TCHECK(!s8_sw.startsWith("world"));
    TCHECK(s8_sw.endsWith(".cpp"));

           // substr
    lString8 s8_sub2("hello world");
    TCHECK(s8_sub2.substr(6) == "world");
    TCHECK(s8_sub2.substr(0, 5) == "hello");

           // operator[]
    TCHECK(s8_sub2[0] == 'h');
    TCHECK(s8_sub2[4] == 'o');

           // data / c_str
    TCHECK(strcmp(s8_sub2.c_str(), "hello world") == 0);
    TCHECK(strcmp(s8_sub2.data(), "hello world") == 0);

           // length / size / capacity / empty
    TCHECK(s8_sub2.length() == 11);
    TCHECK(s8_sub2.size() == 11);
    TCHECK(!s8_sub2.empty());
    lString8 s8_empty;
    TCHECK(s8_empty.empty());
    TCHECK(!s8_empty);

           // at
    TCHECK(s8_sub2.at(0) == 'h');
    TCHECK(s8_sub2.at(10) == 'd');

           // atoi
    lString8 s8_int("12345");
    TCHECK(s8_int.atoi() == 12345);
    lString8 s8_int2("-999");
    TCHECK(s8_int2.atoi() == -999);
    lString8 s8_int64("1234567890123");
    TCHECK(s8_int64.atoi64() == 1234567890123LL);

           // swap
    lString8 s8_swap1("first");
    lString8 s8_swap2("second");
    s8_swap1.swap(s8_swap2);
    TCHECK(s8_swap1 == "second");
    TCHECK(s8_swap2 == "first");

           // trim
    lString8 s8_trim("  spaced  ");
    s8_trim.trim();
    TCHECK(s8_trim == "spaced");

           // pack
    lString8 s8_pack("hello");
    s8_pack.reserve(1000);
    s8_pack.pack();

           // clear / reset
    lString8 s8_clr("something");
    s8_clr.clear();
    TCHECK(s8_clr.empty());
    lString8 s8_rst("something");
    s8_rst.reset(50);
    TCHECK(s8_rst.empty());
    TCHECK(s8_rst.capacity() >= 49);

           // resize
    lString8 s8_rsz("hi");
    s8_rsz.resize(5, 'x');

           // lastChar / firstChar
    lString8 s8_lf("abc");
    TCHECK(s8_lf.firstChar() == 'a');
    TCHECK(s8_lf.lastChar() == 'c');

           // getHash
    lString8 s8_h1("hello");
    lString8 s8_h2("hello");
    TCHECK(s8_h1.getHash() == s8_h2.getHash());

           // operator <<
    lString8 s8_shift;
    s8_shift << 'A';
    TCHECK(s8_shift == "A");
    s8_shift << "BC";
    TCHECK(s8_shift == "ABC");
    s8_shift << lString8("DE");
    TCHECK(s8_shift == "ABCDE");
    s8_shift.clear();
    s8_shift << fmt::decimal(42);
    TCHECK(s8_shift == "42");
    s8_shift.clear();
    s8_shift << fmt::hex(255);
    TCHECK(s8_shift == "ff");

           // operator +=
    lString8 s8_plus;
    s8_plus += "hello";
    TCHECK(s8_plus == "hello");
    s8_plus += lString8(" world");
    TCHECK(s8_plus == "hello world");
    s8_plus += '!';
    TCHECK(s8_plus == "hello world!");
    s8_plus += fmt::decimal(99);
    TCHECK(s8_plus == "hello world!99");

           // external operator == / !=
    lString8 s8_eq("equal");
    lString8 s8_eq2("equal");
    lString8 s8_neq("not_equal");
    TCHECK(s8_eq == s8_eq2);
    TCHECK(!(s8_eq != s8_eq2));
    TCHECK(s8_eq != s8_neq);
    TCHECK(s8_eq == "equal");
    TCHECK("equal" == s8_eq);
    TCHECK(s8_eq != "other");
    TCHECK("other" != s8_eq);

           // external operator +
    lString8 s8_add("abc");
    lString8 s8_added = s8_add + "def";
    TCHECK(s8_added == "abcdef");
    s8_added = s8_add + lString8("ghi");
    TCHECK(s8_added == "abcghi");
    s8_added = s8_add + fmt::decimal(123);
    TCHECK(s8_added == "abc123");
    s8_added = s8_add + fmt::hex(0xFF);
    TCHECK(s8_added == "abcff");

           // itoa static
    lString8 s8_i = lString8::itoa(42);
    TCHECK(s8_i == "42");
    s8_i = lString8::itoa(0U);
    TCHECK(s8_i == "0");
    s8_i = lString8::itoa(-123LL);
    TCHECK(s8_i == "-123");

           // cs8
    const lString8& cs8_test = cs8("static_string");
    TCHECK(cs8_test == "static_string");
    const lString8& cs8_test2 = cs8("static_string");
    TCHECK(&cs8_test == &cs8_test2);
}

void testStrings16() {
    printf("=== lString16 tests ===\n");

           // lString16 constructors
    lString16 s16_default;
    TCHECK(s16_default.empty());
    lString16 s16_cstr(u"hello");
    TCHECK(s16_cstr.length() == 5);
    lString16 s16_from8("hello");
    TCHECK(s16_from8.length() == 5);
    lString16 s16_frag(u"hello world", 5);
    TCHECK(s16_frag == u"hello");
    lString16 s16_sub(s16_cstr, 1, 3);
    TCHECK(s16_sub == u"ell");

           // Copy / move
    lString16 s16_copy(s16_cstr);
    TCHECK(s16_copy == u"hello");
    lString16 s16_move(std::move(s16_copy));
    TCHECK(s16_move == u"hello");

           // Assignment
    lString16 s16_assign;
    s16_assign = u"world";
    TCHECK(s16_assign == u"world");
    s16_assign = "ascii";
    TCHECK(s16_assign == u"ascii");

           // Append
    lString16 s16_app;
    s16_app.append(u"foo");
    TCHECK(s16_app == u"foo");
    s16_app.append((const lChar8*)"bar");
    TCHECK(s16_app == u"foobar");
    s16_app.append(3, u'!');
    TCHECK(s16_app == u"foobar!!!");
    s16_app.clear();

           // appendDecimal / appendHex
    s16_app.appendDecimal(999);
    TCHECK(s16_app == u"999");
    s16_app.clear();
    s16_app.appendHex(0xABC);
    TCHECK(s16_app == u"abc");

           // Insert
    lString16 s16_ins(u"world");
    s16_ins.insert(0, 2, u'X');
    s16_ins.clear();
    s16_ins = u"world";
    s16_ins.insert(0, lString16(u"hello "));

           // compare
    TCHECK(s16_cstr.compare(u"hello") == 0);
    TCHECK(s16_cstr.compare((const lChar8*)"hello") == 0);
    TCHECK(s16_cstr.compare(u"world") < 0);

           // substr
    lString16 s16_sub3(u"hello world");
    TCHECK(s16_sub3.substr(6) == u"world");
    TCHECK(s16_sub3.substr(0, 5) == u"hello");

           // startsWith / endsWith
    TCHECK(s16_sub3.startsWith(u"hello"));
    TCHECK(s16_sub3.startsWith((const lChar8*)"hello"));
    TCHECK(s16_sub3.endsWith(u"world"));
    TCHECK(s16_sub3.endsWith((const lChar8*)"world"));
    TCHECK(s16_sub3.endsWith(lString16(u"world")));

           // operator[]
    TCHECK(s16_sub3[0] == u'h');

           // atoi
    lString16 s16_int(u"6789");
    int s16_ival = 0;
    lInt64 s16_ival64 = 0;
    TCHECK(s16_int.atoi() == 6789);
    TCHECK(s16_int.atoi(s16_ival) && s16_ival == 6789);
    TCHECK(s16_int.atoi(s16_ival64) && s16_ival64 == 6789);

           // getHash
    lString16 s16_h1(u"test");
    lString16 s16_h2(u"test");
    TCHECK(s16_h1.getHash() == s16_h2.getHash());

           // trim / trimNonAlpha
    lString16 s16_tr(u"  spaced  ");
    s16_tr.trim();
    TCHECK(s16_tr == u"spaced");
    lString16 s16_tna(u"!!!hello!!!");
    s16_tna.trimNonAlpha();
    TCHECK(s16_tna == u"hello");

           // swap
    lString16 s16_sw1(u"first");
    lString16 s16_sw2(u"second");
    s16_sw1.swap(s16_sw2);
    TCHECK(s16_sw1 == u"second");
    TCHECK(s16_sw2 == u"first");

           // clear / reset
    lString16 s16_clr(u"something");
    s16_clr.clear();
    TCHECK(s16_clr.empty());
    lString16 s16_rst2(u"something");
    s16_rst2.reset(50);
    TCHECK(s16_rst2.empty());

           // limit
    lString16 s16_lim(u"hello world");
    s16_lim.limit(5);
    TCHECK(s16_lim == u"hello");

           // pack
    lString16 s16_pk(u"test");
    s16_pk.reserve(500);

           // operator <<
    lString16 s16_sh;
    s16_sh << u'A';
    TCHECK(s16_sh == u"A");
    s16_sh << u"BC";
    TCHECK(s16_sh == u"ABC");
    s16_sh << fmt::decimal(42);
    TCHECK(s16_sh == u"ABC42");
    s16_sh.clear();
    s16_sh << fmt::hex(255);
    TCHECK(s16_sh == u"ff");

           // operator +=
    lString16 s16_pe;
    s16_pe += u"hello";
    TCHECK(s16_pe == u"hello");
    s16_pe += lString16(u" world");
    TCHECK(s16_pe == u"hello world");

           // itoa
    lString16 s16_i16 = lString16::itoa(42);
    TCHECK(s16_i16 == u"42");
    s16_i16 = lString16::itoa(0U);
    TCHECK(s16_i16 == u"0");

           // external operator == / != / +
    lString16 s16_eq1(u"equal");
    lString16 s16_eq2(u"equal");
    TCHECK(s16_eq1 == s16_eq2);
    TCHECK(s16_eq1 == u"equal");
    TCHECK(u"equal" == s16_eq1);
    TCHECK(s16_eq1 != u"other");
    lString16 s16_added16 = s16_eq1 + u"_suffix";
    TCHECK(s16_added16 == u"equal_suffix");
}

void testStrings32() {
    printf("=== lString32 tests ===\n");

           // lString32 constructors
    lString32 s32_default;
    TCHECK(s32_default.empty());
    lString32 s32_cstr(U"hello");
    TCHECK(s32_cstr.length() == 5);
    lString32 s32_from8("hello");
    TCHECK(s32_from8.length() == 5);
    lString32 s32_frag(U"hello world", 5);
    TCHECK(s32_frag == U"hello");
    lString32 s32_sub(s32_cstr, 1, 3);
    TCHECK(s32_sub == U"ell");

           // Copy / move
    lString32 s32_copy(s32_cstr);
    TCHECK(s32_copy == U"hello");
    lString32 s32_move(std::move(s32_copy));
    TCHECK(s32_move == U"hello");

           // Assignment
    lString32 s32_assign;
    s32_assign = U"world";
    TCHECK(s32_assign == U"world");
    s32_assign = "ascii";
    TCHECK(s32_assign == U"ascii");

           // Append
    lString32 s32_app;
    s32_app.append(U"foo");
    TCHECK(s32_app == U"foo");
    s32_app.append((const lChar8*)"bar");
    TCHECK(s32_app == U"foobar");
    s32_app.append(3, U'!');
    TCHECK(s32_app == U"foobar!!!");
    s32_app.clear();
    s32_app.append(lString32(U"test"));
    TCHECK(s32_app == U"test");

           // appendDecimal / appendHex
    s32_app.appendDecimal(12345);
    TCHECK(s32_app == U"test12345");
    s32_app.clear();
    s32_app.appendHex(0xFF);
    TCHECK(s32_app == U"ff");

           // Insert
    lString32 s32_ins(U"world");
    s32_ins.insert(0, lString32(U"hello "));
    TCHECK(s32_ins == U"hello world");

           // Replace
    lString32 s32_rp(U"ab_ab_ab");
    s32_rp.replace(U"ab", U"X");

           // Replace (range)
    lString32 s32_rpr(U"hello world");
    s32_rpr.replace(0, 5, lString32(U"good"));

           // Erase
    lString32 s32_era(U"0123456789");
    s32_era.erase(5, 5);
    TCHECK(s32_era == U"01234");

           // uppercase / lowercase / capitalize
    lString32 s32_case(U"hello world");
    s32_case.uppercase();
    TCHECK(s32_case == U"HELLO WORLD");
    s32_case.lowercase();
    TCHECK(s32_case == U"hello world");
    s32_case.capitalize();
    TCHECK(s32_case == U"Hello World");

           // fullWidthChars
    lString32 s32_fw(U"abc");
    s32_fw.fullWidthChars();
    TCHECK(s32_fw.length() == 3);
    TCHECK(s32_fw[0] > 0xFF00);

           // compare
    TCHECK(s32_cstr.compare(U"hello") == 0);
    TCHECK(s32_cstr.compare((const lChar8*)"hello") == 0);
    TCHECK(s32_cstr.compare(U"world") < 0);

           // split2
    lString32 s32_sp(U"key=value");
    lString32 k, v;
    s32_sp.split2(U"=", k, v);

           // pos / rpos
    lString32 s32_pos(U"hello world, hello");
    TCHECK(s32_pos.pos(U'w') == 6);
    TCHECK(s32_pos.pos(U"world") == 6);
    TCHECK(s32_pos.pos(U'h', 1) == 13);
    TCHECK(s32_pos.rpos(U"hello") == 13);

           // startsWith / endsWith / startsWithNoCase
    lString32 s32_sw(U"Hello.cpp");
    TCHECK(s32_sw.startsWith(U"Hello"));
    TCHECK(s32_sw.startsWith((const lChar8*)"Hello"));
    TCHECK(s32_sw.startsWith(lString32(U"Hello")));
    TCHECK(s32_sw.endsWith(U".cpp"));
    TCHECK(s32_sw.endsWith((const lChar8*)".cpp"));
    TCHECK(s32_sw.endsWith(lString32(U".cpp")));
    lString32 s32_nc(U"Hello World");
    TCHECK(s32_nc.startsWithNoCase(lString32(U"hello")));

           // substr
    lString32 s32_sub2(U"hello world");
    TCHECK(s32_sub2.substr(6) == U"world");
    TCHECK(s32_sub2.substr(0, 5) == U"hello");

           // atoi / atod
    lString32 s32_int(U"12345");
    TCHECK(s32_int.atoi() == 12345);
    int _i;
    lInt64 _i64;
    double _d;
    TCHECK(s32_int.atoi(_i) && _i == 12345);
    TCHECK(s32_int.atoi(_i64) && _i64 == 12345);
    lString32 s32_dbl(U"3.14");
    TCHECK(s32_dbl.atod() > 3.13 && s32_dbl.atod() < 3.15);
    TCHECK(s32_dbl.atod(_d) && _d > 3.13 && _d < 3.15);

           // operator[]
    TCHECK(s32_sub2[0] == U'h');

           // data / c_str
    lString32 s32_dc(U"test_data");
    TCHECK(s32_dc.c_str() != nullptr);
    TCHECK(s32_dc.data() != nullptr);

           // getHash
    lString32 s32_h1(U"hash");
    lString32 s32_h2(U"hash");
    TCHECK(s32_h1.getHash() == s32_h2.getHash());

           // swap
    lString32 s32_sw1(U"first");
    lString32 s32_sw2(U"second");
    s32_sw1.swap(s32_sw2);
    TCHECK(s32_sw1 == U"second");
    TCHECK(s32_sw2 == U"first");

           // clear / reset
    lString32 s32_clr(U"something");
    s32_clr.clear();
    TCHECK(s32_clr.empty());
    lString32 s32_rst3(U"something");
    s32_rst3.reset(50);
    TCHECK(s32_rst3.empty());

           // resize / limit
    lString32 s32_rsz(U"hi");
    s32_rsz.resize(5, U'x');
    TCHECK(s32_rsz == U"hixxx");
    lString32 s32_lim(U"hello world");
    s32_lim.limit(5);
    TCHECK(s32_lim == U"hello");

           // trim / trimNonAlpha / trimDoubleSpaces
    lString32 s32_tr(U"  spaced  ");
    s32_tr.trim();
    TCHECK(s32_tr == U"spaced");
    lString32 s32_tna(U"!!!hello!!!");
    s32_tna.trimNonAlpha();
    TCHECK(s32_tna == U"hello");
    lString32 s32_tds(U"a  b   c");
    s32_tds.trimDoubleSpaces(false, false);
    TCHECK(s32_tds == U"a b c");

           // pack
    lString32 s32_pk2(U"test");
    s32_pk2.reserve(500);
    TCHECK(s32_pk2.capacity() >= 500);
    s32_pk2.pack();
    TCHECK(s32_pk2.capacity() < 100);

           // operator <<
    lString32 s32_sh;
    s32_sh << U'A';
    TCHECK(s32_sh == U"A");
    s32_sh << U"BC";
    TCHECK(s32_sh == U"ABC");
    s32_sh << fmt::decimal(42);
    TCHECK(s32_sh == U"ABC42");
    s32_sh.clear();
    s32_sh << fmt::hex(255);
    TCHECK(s32_sh == U"ff");

           // operator +=
    lString32 s32_pe;
    s32_pe += U"hello";
    TCHECK(s32_pe == U"hello");
    s32_pe += lString32(U" world");
    TCHECK(s32_pe == U"hello world");

           // replaceParam / replaceIntParam
    lString32 s32_rp2(U"Hello $1, you have $2 messages");
    TCHECK(s32_rp2.replaceParam(1, U"Alice"));
    TCHECK(s32_rp2.replaceIntParam(2, 5));
    TCHECK(s32_rp2 == U"Hello Alice, you have 5 messages");

           // itoa
    lString32 s32_i32 = lString32::itoa(42);
    TCHECK(s32_i32 == U"42");
    s32_i32 = lString32::itoa(0U);
    TCHECK(s32_i32 == U"0");
    s32_i32 = lString32::itoa(-99LL);
    TCHECK(s32_i32 == U"-99");
    s32_i32 = lString32::itoa(0xFFFFFFFFFFFFFFFFULL);
    TCHECK(!s32_i32.empty());

           // cs32
    const lString32& cs32_test = cs32("static_str32");
    TCHECK(cs32_test == U"static_str32");
    const lString32& cs32_test2 = cs32(U"wide_static");
    TCHECK(cs32_test2 == U"wide_static");

           // external operator == / != / +
    lString32 s32_eq1(U"equal");
    lString32 s32_eq2(U"equal");
    TCHECK(s32_eq1 == s32_eq2);
    TCHECK(s32_eq1 == U"equal");
    TCHECK(U"equal" == s32_eq1);
    TCHECK(s32_eq1 != U"other");
    lString32 s32_added32 = s32_eq1 + U"_suffix";
    TCHECK(s32_added32 == U"equal_suffix");
    s32_added32 = s32_eq1 + lString32(U"_extra");
    TCHECK(s32_added32 == U"equal_extra");
    s32_added32 = s32_eq1 + fmt::decimal(123);
    TCHECK(s32_added32 == U"equal123");
    s32_added32 = s32_eq1 + fmt::hex(0xFF);
    TCHECK(s32_added32 == U"equalff");

           // Mixed-type comparisons
    lString32 s32_mix(U"test");
    lString16 s16_mix(u"test");
    lString8 s8_mix("test");
    TCHECK(s32_mix == s16_mix);
    TCHECK(s32_mix == s8_mix);
    TCHECK(s16_mix == s32_mix);

           // External conversion functions
    lString32 s32_conv(U"Hello World");
    lString8 s8_utf8 = UnicodeToUtf8(s32_conv);
    TCHECK(s8_utf8 == "Hello World");
    lString16 s16_utf16 = UnicodeToUtf16(s32_conv);
    TCHECK(s16_utf16 == u"Hello World");
    lString8 s8_local = UnicodeToLocal(s32_conv);
    TCHECK(!s8_local.empty());
    lString8 s8_wtf8 = UnicodeToWtf8(s32_conv);
    TCHECK(s8_wtf8 == "Hello World");

           // Reverse conversion
    lString32 s32_back = Utf8ToUnicode(s8_utf8);
    TCHECK(s32_back == U"Hello World");
    s32_back = Utf8ToUnicode("direct_cstr");
    TCHECK(s32_back == U"direct_cstr");
    s32_back = Utf8ToUnicode("fragment", 4);
    TCHECK(s32_back == U"frag");
    s32_back = Utf16ToUnicode(s16_utf16);
    TCHECK(s32_back == U"Hello World");
    s32_back = Utf16ToUnicode(u"direct16");
    TCHECK(s32_back == U"direct16");
    lString32 s32_wtf = Wtf8ToUnicode(s8_wtf8);
    TCHECK(s32_wtf == U"Hello World");
    s32_wtf = Wtf8ToUnicode("wtf_direct");
    TCHECK(s32_wtf == U"wtf_direct");

           // external getHash
    lString32 s32_gh(U"hash_me");
    lString16 s16_gh(u"hash_me");
    lString8 s8_gh("hash_me");
    TCHECK(getHash(s32_gh) == s32_gh.getHash());
    TCHECK(getHash(s16_gh) == s16_gh.getHash());
    TCHECK(getHash(s8_gh) == s8_gh.getHash());

}

void testStringsMixed() {

}

void testStrings() {

    testStrings8_16();
    testStrings8();
    testStrings16();
    testStrings32();
    testStringsMixed();


    if (test_errors == 0)
        printf("All string tests passed!\n");
    else
        printf("FAILED: %d test(s) failed!\n", test_errors);
}

void testStringCollections() {
    {
        lString8Collection list;
        {
            lString8 s {"test_s_1"};
            assert(s == "test_s_1");
            list.add(s);
            assert(s == "test_s_1");
        }
        {
            lString8 s {"test_s_2"};
            assert(s == "test_s_2");
            list.add(s);
            assert(s == "test_s_2");
        }
    }
    {
        lString8Collection list;
        {
            lString8 s {"test_s_3"};
            assert(s == "test_s_3");
            list.add(s);
            assert(s == "test_s_3");
        }
        {
            lString8 s {"test_s_4"};
            assert(s == "test_s_4");
            list.add(s);
            assert(s == "test_s_4");
        }
    }
    {
        lString32Collection list;
        {
            lString32 s {U"test_s_5"};
            assert(s == "test_s_5");
            list.add(s);
            assert(s == "test_s_5");
        }
        {
            lString32 s {U"test_s_5"};
            assert(s == "test_s_5");
            list.add(s);
            assert(s == "test_s_5");
        }
    }

    for (int sz = 1; sz < 1000; sz++) {
        lString8Collection list;
        for (int i = 0; i < sz; i++) {
            lString8 s {"String8"};
            list.add(s);
        }
    }
    for (int sz = 1; sz < 1000; sz++) {
        lString32Collection list;
        for (int i = 0; i < sz; i++) {
            lString32 s {U"String32"};
            list.add(s);
        }
    }
}
