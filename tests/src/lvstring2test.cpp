#include "lvstring2test.h"
#include <stdio.h>

// Definition of static empty_str for template instantiations
namespace lv {
template<> const string<lChar8, std::atomic_int> string<lChar8, std::atomic_int>::empty_str;
template<> const string<lChar16, std::atomic_int> string<lChar16, std::atomic_int>::empty_str;
template<> const string<lChar32, std::atomic_int> string<lChar32, std::atomic_int>::empty_str;
}

namespace lv {
static int test_errors = 0;
#define TCHECK(cond) do { if (!(cond)) { printf("LS FAIL line %d: %s\n", __LINE__, #cond); test_errors++; } } while(0)

#ifdef DEBUG_TRACK_LSTRING2_ALLOC
    #define DUMP_ALLOC_STATS(x) ls_alloc_stats.dump(x);
#else
    #define DUMP_ALLOC_STATS(x)
#endif

void test_lstring2_chunks() {

           // struct size checks
    printf("sizeof(lstring8_chunk_t) = %llu\n", sizeof(lstring8_chunk_t));
    printf("sizeof(lstring16_chunk_t) = %llu\n", sizeof(lstring16_chunk_t));
    printf("sizeof(lstring32_chunk_t) = %llu\n", sizeof(lstring32_chunk_t));
    TCHECK(sizeof(lstring8_chunk_t) == sizeof(lstring16_chunk_t));
    TCHECK(sizeof(lstring16_chunk_t) == sizeof(lstring32_chunk_t));
    TCHECK(sizeof(lstring8_chunk_t) == 12);

           //std::atomic_int counter{0};
           //counter.fetch_add();

    TCHECK(lstring8_chunk_t::alloc_align_bytes == 16);
    TCHECK(lstring8_chunk_t::min_size == 3);
    TCHECK(lstring8_chunk_t::size_align == 16);

    TCHECK(lstring16_chunk_t::alloc_align_bytes == 16);
    TCHECK(lstring16_chunk_t::min_size == 1);
    TCHECK(lstring16_chunk_t::size_align == 8);

    TCHECK(lstring32_chunk_t::alloc_align_bytes == 16);
    TCHECK(lstring32_chunk_t::min_size == 0);
    TCHECK(lstring32_chunk_t::size_align == 4);

    TCHECK(lstring8_chunk_t::alignSize(0) == 4 - 1);
    TCHECK(lstring8_chunk_t::alignSize(1) == 4 - 1);
    TCHECK(lstring8_chunk_t::alignSize(2) == 4 - 1);
    TCHECK(lstring8_chunk_t::alignSize(3) == 4 - 1);
    TCHECK(lstring8_chunk_t::alignSize(4) == 16+3);
    TCHECK(lstring8_chunk_t::alignSize(5) == 16+3);
    TCHECK(lstring8_chunk_t::alignSize(6) == 16+3);
    TCHECK(lstring8_chunk_t::alignSize(7) == 16+3);
    TCHECK(lstring8_chunk_t::alignSize(11) == 16+3);
    TCHECK(lstring8_chunk_t::alignSize(12) == 16+3);
    TCHECK(lstring8_chunk_t::alignSize(13) == 16+3);
    TCHECK(lstring8_chunk_t::alignSize(15) == 16+3);
    TCHECK(lstring8_chunk_t::alignSize(16) == 16+3);
    TCHECK(lstring8_chunk_t::alignSize(16+3) == 16+3);
    TCHECK(lstring8_chunk_t::alignSize(16+4) == 16+16+3);

    lstring8_chunk_t * chunk8_1 = lstring8_chunk_t::alloc(25);
    TCHECK(chunk8_1->size == 32+3);
    TCHECK(chunk8_1->len == 0);
    TCHECK(chunk8_1->refCount == 1);
    TCHECK(chunk8_1->buf[0] == 0);
    for (unsigned i = 0; i < 25; i++) {
        chunk8_1->buf[i] = 'a';
        chunk8_1->buf[i + 1] = 0;
    }
    chunk8_1->len = 25;

    chunk8_1->testAddRef();
    chunk8_1->testReleaseRef();

    lstring8_chunk_t::free(chunk8_1);

    lstring8_chunk_t * chunk8_2 = lstring8_chunk_t::alloc("abcdefg", 4, 10);
    TCHECK(chunk8_2->size == 16+3);
    TCHECK(chunk8_2->len == 4);
    TCHECK(chunk8_2->refCount == 1);
    TCHECK(chunk8_2->buf[0] == 'a' && chunk8_2->buf[1] == 'b' && chunk8_2->buf[2] == 'c' && chunk8_2->buf[3] == 'd' && chunk8_2->buf[4] == 0);
    lstring8_chunk_t::free(chunk8_2);

    lstring16_chunk_t * chunk16_1 = lstring16_chunk_t::alloc(25);
    TCHECK(chunk16_1->size == 24+1);
    TCHECK(chunk16_1->len == 0);
    TCHECK(chunk16_1->refCount == 1);
    TCHECK(chunk16_1->buf[0] == 0);
    for (unsigned i = 0; i < 25; i++) {
        chunk16_1->buf[i] = 'b';
        chunk16_1->buf[i + 1] = 0;
    }
    chunk16_1->len = 25;
    lstring16_chunk_t::free(chunk16_1);

    lstring16_chunk_t * chunk16_2 = lstring16_chunk_t::alloc(u"bcdefghi", 4, 10);
    TCHECK(chunk16_2->size == 16+1);
    TCHECK(chunk16_2->len == 4);
    TCHECK(chunk16_2->refCount == 1);
    TCHECK(chunk16_2->buf[0] == u'b' && chunk16_2->buf[1] == u'c' && chunk16_2->buf[2] == u'd' && chunk16_2->buf[3] == u'e' && chunk16_2->buf[4] == 0);
    intrusive_ptr_add_ref(chunk16_2);
    TCHECK(chunk16_2->getRefCount() == 2);
    intrusive_ptr_release(chunk16_2);
    TCHECK(chunk16_2->getRefCount() == 1);
    intrusive_ptr_release(chunk16_2);
    // released automatically
    //lstring16_chunk_t::free(chunk16_2);

    lstring32_chunk_t * chunk32_1 = lstring32_chunk_t::alloc(25);
    TCHECK(chunk32_1->size == 28);
    TCHECK(chunk32_1->len == 0);
    TCHECK(chunk32_1->refCount == 1);
    TCHECK(chunk32_1->buf[0] == 0);
    for (unsigned i = 0; i < 25; i++) {
        chunk32_1->buf[i] = 'b';
        chunk32_1->buf[i + 1] = 0;
    }
    chunk32_1->len = 25;
    //lstring32_chunk_t::free(chunk32_1);
    intrusive_ptr_release(chunk32_1);

    lstring32_chunk_t * chunk32_2 = lstring32_chunk_t::alloc(U"cdefghijkl", 4, 10);
    TCHECK(chunk32_2->size == 12);
    TCHECK(chunk32_2->len == 4);
    TCHECK(chunk32_2->refCount == 1);
    TCHECK(chunk32_2->buf[0] == U'c' && chunk32_2->buf[1] == U'd' && chunk32_2->buf[2] == U'e' && chunk32_2->buf[3] == U'f' && chunk32_2->buf[4] == 0);
    lstring32_chunk_t::free(chunk32_2);

}

void test_lstring8() {
    printf("running test_lstring8()\n");
    DUMP_ALLOC_STATS("Entering test_lstring8()")

           // --- Default construction ---
    lString8 s {};
    TCHECK(s.capacity() == 0);
    TCHECK(s.size() == 0);
    TCHECK(s.length() == 0);
    TCHECK(s.empty());

           // --- C-string construction ---
    lString8 s2 {"qwerty"};
    TCHECK(s2.capacity() >= 6);
    TCHECK(s2.size() >= 6);
    TCHECK(s2.length() == 6);
    TCHECK(!s2.empty());

           // --- C-string with count + reserved ---
    lString8 s3 {"qwerty", 3, 100};
    TCHECK(s3.capacity() >= 100);
    TCHECK(s3.size() >= 100);
    TCHECK(s3.length() == 3);
    TCHECK(!s3.empty());

           // --- Move construction ---
    lString8 s4 = lString8("move");
    TCHECK(s4.length() == 4);
    s4 = lString8("move consturctor assignment");
    TCHECK(s4.length() == 27);

           // --- Copy assignment ---
    s2 = s4;
    TCHECK(s2 == s4);
    TCHECK(s2.length() == s4.length());

           // --- Clear ---
    DUMP_ALLOC_STATS("before clear")
    s2.clear();
    DUMP_ALLOC_STATS("after clear")
    TCHECK(s2.empty());
    TCHECK(s2.length() == 0);
    TCHECK(s2.capacity() == 0);

           // --- Reset ---
    s4.reset(500);
    DUMP_ALLOC_STATS("after reset")
    TCHECK(s4.empty());
    TCHECK(s4.length() == 0);
    TCHECK(s4.capacity() >= 500);

           // --- Reset on empty ---
    lString8 s_empty;
    s_empty.reset(50);
    TCHECK(s_empty.empty());
    TCHECK(s_empty.capacity() >= 50);

           // --- Resize (expands length, fills with char) ---
    lString8 s_rsz {"hi"};
    s_rsz.resize(5, 'x');
    TCHECK(s_rsz.length() == 5);
    TCHECK(s_rsz[0] == 'h');
    TCHECK(s_rsz[1] == 'i');
    TCHECK(s_rsz[2] == 'x');
    TCHECK(s_rsz[3] == 'x');
    TCHECK(s_rsz[4] == 'x');

           // --- Resize to smaller (truncates length) ---
    lString8 s_rsz2 {"hello world"};
    s_rsz2.resize(3);
    TCHECK(s_rsz2.length() == 3);
    TCHECK(s_rsz2[0] == 'h');
    TCHECK(s_rsz2[1] == 'e');
    TCHECK(s_rsz2[2] == 'l');

           // --- Resize empty string ---
    lString8 s_rsz3;
    s_rsz3.resize(4, 'z');
    TCHECK(s_rsz3.length() == 4);
    TCHECK(s_rsz3[0] == 'z');
    TCHECK(s_rsz3[3] == 'z');

           // --- Comparison (string vs string) ---
    lString8 s5 {"abc"};
    lString8 s6 {"abc"};
    lString8 s7 {"Abc"};
    DUMP_ALLOC_STATS("before compare")
    TCHECK(s5 == s6);
    TCHECK(s5 <= s6);
    TCHECK(s5 >= s6);
    TCHECK(s5 > s7);
    TCHECK(s7 < s5);
    TCHECK(s5 >= s7);
    TCHECK(s7 <= s5);
    TCHECK(s5 != s7);

           // --- Comparison (string vs c-string) ---
    TCHECK(s5 == "abc");
    TCHECK(s5 > "ab");
    TCHECK(s5 < "abcd");
    TCHECK(s5 != "xyz");
    TCHECK(s5 <= "abc");
    TCHECK(s5 >= "abc");
    TCHECK(s5 <= "abd");
    TCHECK(s5 >= "abb");

           // --- Comparison with empty ---
    lString8 s_empty2;
    TCHECK(s_empty2 == "");
    TCHECK(s_empty2 == nullptr);
    TCHECK(s_empty2 < "a");
    TCHECK(s5 > "");

           // --- Comparison with nullptr ---
    TCHECK(s_empty2 == nullptr);
    TCHECK(s5 != nullptr);

           // --- Self-assignment ---
    lString8 s_self {"self"};
    lString8 &ref_self = s_self;
    s_self = ref_self;
    TCHECK(s_self == "self");

           // --- Move assignment to self (same pointer) ---
    lString8 s_selfmove {"selfmove"};
    s_selfmove = std::move(s_selfmove);
    TCHECK(s_selfmove == "selfmove");

           // --- Compare method ---
    TCHECK(s5.compare(s6) == 0);
    TCHECK(s5.compare("abc") == 0);
    TCHECK(s5.compare("abd") < 0);
    TCHECK(s5.compare("abb") > 0);

           // --- Length/size/capacity/empty consistency ---
    lString8 s_len {"12345"};
    TCHECK(s_len.length() == 5);
    TCHECK(s_len.size() == s_len.capacity());
    TCHECK(!s_len.empty());
    TCHECK(lString8{}.empty());
    TCHECK(lString8{}.length() == 0);
    TCHECK(lString8{}.capacity() == 0);

           // --- Edge case: empty string construction ---
    lString8 s_empty_str {""};
    TCHECK(s_empty_str.empty());
    TCHECK(s_empty_str.length() == 0);

           // --- Edge case: nullptr construction ---
    lString8 s_null {nullptr};
    TCHECK(s_null.empty());
    TCHECK(s_null.length() == 0);
    TCHECK(s_null.capacity() == 0);

           // --- Edge case: single char ---
    lString8 s_one {"a"};
    TCHECK(s_one.length() == 1);
    TCHECK(s_one[0] == 'a');

           // --- Edge case: string with embedded null (count constructor) ---
    lString8 s_embed {"ab\0cd", 5, 5};
    TCHECK(s_embed.length() == 5);
    TCHECK(s_embed.capacity() >= 5);

           // --- Copy constructor ---
    lString8 s_copy {"copyme"};
    lString8 s_copy2 {s_copy};
    TCHECK(s_copy2 == "copyme");
    TCHECK(s_copy2.length() == 6);

           // --- Move constructor ---
    lString8 s_move_src {"moveme"};
    lString8 s_move_dst {std::move(s_move_src)};
    TCHECK(s_move_dst == "moveme");
    TCHECK(s_move_dst.length() == 6);
    // s_move_src is now in moved-from state

    DUMP_ALLOC_STATS("after compare")

           // --- Reserve on empty string ---
    lString8 s_rsv_empty;
    TCHECK(s_rsv_empty.empty());
    TCHECK(s_rsv_empty.capacity() == 0);
    s_rsv_empty.reserve(0);
    TCHECK(s_rsv_empty.empty());
    TCHECK(s_rsv_empty.capacity() == 0);

    s_rsv_empty.reserve(100);
    TCHECK(s_rsv_empty.empty());
    TCHECK(s_rsv_empty.length() == 0);
    TCHECK(s_rsv_empty.capacity() >= 100);

    // --- Reserve on non-empty owned string ---
    lString8 s_rsv_own {"hello"};
    size_t cap_before = s_rsv_own.capacity();
    s_rsv_own.reserve(3);
    TCHECK(s_rsv_own.length() == 5);
    TCHECK(s_rsv_own == "hello");
    TCHECK(s_rsv_own.capacity() == cap_before);  // no shrink

    s_rsv_own.reserve(cap_before);
    TCHECK(s_rsv_own.capacity() == cap_before);  // exact match, no realloc

    s_rsv_own.reserve(cap_before + 50);
    TCHECK(s_rsv_own.length() == 5);
    TCHECK(s_rsv_own == "hello");
    TCHECK(s_rsv_own.capacity() >= cap_before + 50);

    // --- Reserve on shared string (forces copy) ---
    lString8 s_rsv_src {"shared"};
    lString8 s_rsv_copy = s_rsv_src;
    TCHECK(s_rsv_src.length() == 6);
    TCHECK(s_rsv_copy.length() == 6);
    s_rsv_copy.reserve(100);
    TCHECK(s_rsv_copy.capacity() >= 100);
    TCHECK(s_rsv_copy.length() == 6);
    TCHECK(s_rsv_copy == "shared");
    // original unchanged
    TCHECK(s_rsv_src.length() == 6);
    TCHECK(s_rsv_src == "shared");

    // --- Reserve on shared string with requested size < len ---
    lString8 s_rsv_src2 {"longer string"};
    lString8 s_rsv_copy2 = s_rsv_src2;
    s_rsv_copy2.reserve(1);
    TCHECK(s_rsv_copy2.length() == 13);
    TCHECK(s_rsv_copy2 == "longer string");
    TCHECK(s_rsv_copy2.capacity() >= 13);

    // --- Reserve(0) on non-empty string ---
    lString8 s_rsv_nz {"test"};
    s_rsv_nz.reserve(0);
    TCHECK(s_rsv_nz.length() == 4);
    TCHECK(s_rsv_nz == "test");
    // capacity should not shrink
    TCHECK(s_rsv_nz.capacity() >= 4);

           // --- c_str() on empty string ---
    lString8 s_cstr_empty;
    const char * p_empty = s_cstr_empty.c_str();
    TCHECK(p_empty != nullptr);
    TCHECK(p_empty[0] == 0);

           // --- c_str() on non-empty string ---
    lString8 s_cstr {"hello"};
    const char * p_cstr = s_cstr.c_str();
    TCHECK(p_cstr != nullptr);
    TCHECK(strcmp(p_cstr, "hello") == 0);
    TCHECK(p_cstr[5] == 0);

           // --- c_str() enforces null-termination ---
    lString8 s_cstr2 {"abc"};
    // Manually corrupt the terminator (simulating a bug)
    // c_str() should restore it
    const char * p_cstr2 = s_cstr2.c_str();
    TCHECK(p_cstr2[3] == 0);

           // --- data() const on empty string ---
    lString8 s_data_empty;
    const char * p_dempty = s_data_empty.data();
    TCHECK(p_dempty != nullptr);
    TCHECK(p_dempty[0] == 0);
    TCHECK(p_dempty == s_data_empty.c_str());

           // --- data() const on non-empty string ---
    lString8 s_data {"world"};
    const char * p_data = s_data.data();
    TCHECK(p_data != nullptr);
    TCHECK(strcmp(p_data, "world") == 0);
    TCHECK(p_data == s_data.c_str());

           // --- data() non-const on empty string ---
    lString8 s_data_nc_empty;
    char * p_nc_empty = s_data_nc_empty.data();
    TCHECK(p_nc_empty != nullptr);
    TCHECK(p_nc_empty[0] == 0);

           // --- data() non-const on non-empty owned string ---
    lString8 s_data_nc {"modify"};
    char * p_nc = s_data_nc.data();
    TCHECK(p_nc != nullptr);
    TCHECK(strcmp(p_nc, "modify") == 0);
    p_nc[0] = 'M';
    TCHECK(s_data_nc == "Modify");
    TCHECK(s_data_nc.length() == 6);

           // --- data() non-const on shared string (triggers lock/copy) ---
    lString8 s_data_shared {"shared"};
    lString8 s_data_copy = s_data_shared;
    TCHECK(s_data_shared == "shared");
    TCHECK(s_data_copy == "shared");
    char * p_shared = s_data_copy.data();
    TCHECK(p_shared != nullptr);
    TCHECK(strcmp(p_shared, "shared") == 0);
    p_shared[0] = 'S';
    TCHECK(s_data_copy == "Shared");
    // original must be unchanged
    TCHECK(s_data_shared == "shared");

           // --- data() non-const after multiple shares ---
    lString8 s_multi {"multi"};
    lString8 s_m1 = s_multi;
    lString8 s_m2 = s_multi;
    lString8 s_m3 = s_multi;
    char * p_multi = s_m2.data();
    TCHECK(strcmp(p_multi, "multi") == 0);
    p_multi[0] = 'X';
    TCHECK(s_m2 == "Xulti");
    TCHECK(s_multi == "multi");
    TCHECK(s_m1 == "multi");
    TCHECK(s_m3 == "multi");

           // --- lock() on null string (no-op) ---
    lString8 s_lock_null;
    s_lock_null.lock(100);
    TCHECK(s_lock_null.empty());
    TCHECK(s_lock_null.capacity() == 0);

           // --- lock() on owned string (no-op when refCount==1) ---
    lString8 s_lock_own {"owned"};
    size_t cap_lock_before = s_lock_own.capacity();
    s_lock_own.lock(50);
    TCHECK(s_lock_own == "owned");
    TCHECK(s_lock_own.length() == 5);
    TCHECK(s_lock_own.capacity() == cap_lock_before);  // no realloc

           // --- lock() on shared string with newsize >= len ---
    lString8 s_lock_src {"locktest"};
    lString8 s_lock_cpy = s_lock_src;
    TCHECK(s_lock_src.length() == 8);
    TCHECK(s_lock_cpy.length() == 8);
    s_lock_cpy.lock(20);
    TCHECK(s_lock_cpy == "locktest");
    TCHECK(s_lock_cpy.length() == 8);
    TCHECK(s_lock_cpy.capacity() >= 20);
    // original unchanged
    TCHECK(s_lock_src == "locktest");
    TCHECK(s_lock_src.length() == 8);

           // --- lock() on shared string with newsize < len (ensures full copy) ---
    lString8 s_lock_src2 {"longstring"};
    lString8 s_lock_cpy2 = s_lock_src2;
    s_lock_cpy2.lock(3);
    TCHECK(s_lock_cpy2 == "longstring");
    TCHECK(s_lock_cpy2.length() == 10);
    TCHECK(s_lock_cpy2.capacity() >= 10);
    // original unchanged
    TCHECK(s_lock_src2 == "longstring");

           // --- lock() on already-owned string with newsize < len (no-op) ---
    lString8 s_lock_own2 {"short"};
    s_lock_own2.lock(2);
    TCHECK(s_lock_own2 == "short");
    TCHECK(s_lock_own2.length() == 5);

           // --- data() non-const after lock() ---
    lString8 s_lock_data {"before"};
    lString8 s_lock_data2 = s_lock_data;
    s_lock_data2.lock(50);
    char * p_ld = s_lock_data2.data();
    TCHECK(p_ld != nullptr);
    TCHECK(strcmp(p_ld, "before") == 0);
    p_ld[0] = 'A';
    TCHECK(s_lock_data2 == "Aefore");
    TCHECK(s_lock_data == "before");

           // --- firstChar() / lastChar() on non-empty string ---
    lString8 s_fl {"abc"};
    TCHECK(s_fl.firstChar() == 'a');
    TCHECK(s_fl.lastChar() == 'c');

           // --- firstChar() / lastChar() on single-char string ---
    lString8 s_one2 {"x"};
    TCHECK(s_one2.firstChar() == 'x');
    TCHECK(s_one2.lastChar() == 'x');

           // --- firstChar() / lastChar() on empty string ---
    lString8 s_empty_fl;
    TCHECK(s_empty_fl.firstChar() == 0);
    TCHECK(s_empty_fl.lastChar() == 0);

           // --- firstChar() / lastChar() on shared string ---
    lString8 s_fl_shared {"shared"};
    lString8 s_fl_copy = s_fl_shared;
    TCHECK(s_fl_shared.firstChar() == 's');
    TCHECK(s_fl_shared.lastChar() == 'd');
    TCHECK(s_fl_copy.firstChar() == 's');
    TCHECK(s_fl_copy.lastChar() == 'd');

           // --- firstChar() / lastChar() after modification ---
    lString8 s_fl_mod {"hello"};
    char * p_fl = s_fl_mod.data();
    p_fl[0] = 'H';
    p_fl[4] = 'O';
    TCHECK(s_fl_mod.firstChar() == 'H');
    TCHECK(s_fl_mod.lastChar() == 'O');

           // --- firstChar() / lastChar() with embedded null ---
    lString8 s_fl_null {"ab\0cd", 5, 5};
    TCHECK(s_fl_null.firstChar() == 'a');
    TCHECK(s_fl_null.lastChar() == 'd');
    TCHECK(s_fl_null.length() == 5);

           // --- Fragment constructor ---
    lString8 s_frag_src {"hello world"};
    lString8 s_frag {s_frag_src, 6, 5};
    TCHECK(s_frag == "world");
    TCHECK(s_frag.length() == 5);
    TCHECK(s_frag.capacity() >= 5);

           // --- Fragment constructor: offset beyond length ---
    lString8 s_frag_off {s_frag_src, 100, 5};
    TCHECK(s_frag_off.empty());
    TCHECK(s_frag_off.length() == 0);

           // --- Fragment constructor: count exceeds remaining ---
    lString8 s_frag_cnt {s_frag_src, 6, 100};
    TCHECK(s_frag_cnt == "world");
    TCHECK(s_frag_cnt.length() == 5);

           // --- Fragment constructor: empty source ---
    lString8 s_frag_empty_src;
    lString8 s_frag_empty {s_frag_empty_src, 0, 5};
    TCHECK(s_frag_empty.empty());

           // --- Fragment constructor: zero count ---
    lString8 s_frag_zero {s_frag_src, 0, 0};
    TCHECK(s_frag_zero.empty());

           // --- Fragment assignment ---
    lString8 s_fassign_src {"abcdef"};
    lString8 s_fassign {"initial"};
    s_fassign.assign(s_fassign_src, 2, 3);
    TCHECK(s_fassign == "cde");
    TCHECK(s_fassign.length() == 3);

           // --- Fragment assignment: self-assignment ---
    lString8 s_fassign_self {"selfassign"};
    s_fassign_self.assign(s_fassign_self, 4, 6);
    TCHECK(s_fassign_self == "assign");

           // --- Fragment assignment: offset beyond length ---
    lString8 s_fassign_off {"short"};
    s_fassign_off.assign(s_fassign_off, 100, 5);
    TCHECK(s_fassign_off.empty());

           // --- Fragment assignment: count exceeds remaining ---
    lString8 s_fassign_cnt {"hello"};
    s_fassign_cnt.assign(s_fassign_cnt, 2, 100);
    TCHECK(s_fassign_cnt == "llo");
    TCHECK(s_fassign_cnt.length() == 3);

           // --- Fragment assignment: empty source ---
    lString8 s_fassign_empty_src;
    lString8 s_fassign_empty {"notempty"};
    s_fassign_empty.assign(s_fassign_empty_src, 0, 5);
    TCHECK(s_fassign_empty.empty());

           // --- modify() on empty string ---
    lString8 s_mod_empty;
    char * p_mod_empty = s_mod_empty.modify();
    TCHECK(p_mod_empty != nullptr);
    p_mod_empty[0] = 'H';
    p_mod_empty[1] = 'i';
    p_mod_empty[2] = 0;
    TCHECK(s_mod_empty.length() == 0);  // modify doesn't change length
    TCHECK(s_mod_empty.capacity() >= 8);

           // --- modify() on owned string ---
    lString8 s_mod_own {"owned"};
    char * p_mod_own = s_mod_own.modify();
    TCHECK(strcmp(p_mod_own, "owned") == 0);
    p_mod_own[0] = 'O';
    TCHECK(s_mod_own == "Owned");

           // --- modify() on shared string (triggers copy) ---
    lString8 s_mod_shared {"shared"};
    lString8 s_mod_copy = s_mod_shared;
    char * p_mod_shared = s_mod_copy.modify();
    TCHECK(strcmp(p_mod_shared, "shared") == 0);
    p_mod_shared[0] = 'S';
    TCHECK(s_mod_copy == "Shared");
    TCHECK(s_mod_shared == "shared");  // original unchanged

           // --- modify() after modification ---
    lString8 s_mod_chain {"chain"};
    char * p_mod1 = s_mod_chain.modify();
    p_mod1[0] = 'C';
    char * p_mod2 = s_mod_chain.modify();
    p_mod2[1] = 'H';
    TCHECK(s_mod_chain == "CHain");

           // --- erase() from middle of owned string ---
    lString8 s_erase_mid {"hello world"};
    s_erase_mid.erase(5, 6);
    TCHECK(s_erase_mid == "hello");
    TCHECK(s_erase_mid.length() == 5);

           // --- erase() from beginning of owned string ---
    lString8 s_erase_beg {"hello world"};
    s_erase_beg.erase(0, 6);
    TCHECK(s_erase_beg == "world");
    TCHECK(s_erase_beg.length() == 5);

           // --- erase() from end of owned string ---
    lString8 s_erase_end {"hello world"};
    s_erase_end.erase(5, 100);
    TCHECK(s_erase_end == "hello");
    TCHECK(s_erase_end.length() == 5);

           // --- erase() entire string ---
    lString8 s_erase_all {"abc"};
    s_erase_all.erase(0, 3);
    TCHECK(s_erase_all.empty());
    TCHECK(s_erase_all.length() == 0);

           // --- erase() with zero count (no-op) ---
    lString8 s_erase_zero {"nochange"};
    s_erase_zero.erase(2, 0);
    TCHECK(s_erase_zero == "nochange");

           // --- erase() with offset beyond length (no-op) ---
    lString8 s_erase_off {"short"};
    s_erase_off.erase(100, 5);
    TCHECK(s_erase_off == "short");

           // --- erase() on empty string (no-op) ---
    lString8 s_erase_empty;
    s_erase_empty.erase(0, 5);
    TCHECK(s_erase_empty.empty());

           // --- erase() on shared string (triggers COW copy) ---
    lString8 s_erase_shared {"abcdef"};
    lString8 s_erase_copy = s_erase_shared;
    s_erase_copy.erase(2, 2);
    TCHECK(s_erase_copy == "abef");
    TCHECK(s_erase_copy.length() == 4);
    // original unchanged
    TCHECK(s_erase_shared == "abcdef");
    TCHECK(s_erase_shared.length() == 6);

           // --- erase() single char from middle ---
    lString8 s_erase_one {"abcde"};
    s_erase_one.erase(2, 1);
    TCHECK(s_erase_one == "abde");

           // --- erase() with overlapping tail (memmove test) ---
    lString8 s_erase_overlap {"0123456789"};
    s_erase_overlap.erase(0, 5);
    TCHECK(s_erase_overlap == "56789");

           // --- erase() on shared string, erasing from beginning ---
    lString8 s_erase_sh_beg {"hello world"};
    lString8 s_erase_sh_beg2 = s_erase_sh_beg;
    s_erase_sh_beg2.erase(0, 6);
    TCHECK(s_erase_sh_beg2 == "world");
    TCHECK(s_erase_sh_beg == "hello world");

           // --- erase() on shared string, erasing from end ---
    lString8 s_erase_sh_end {"hello world"};
    lString8 s_erase_sh_end2 = s_erase_sh_end;
    s_erase_sh_end2.erase(5, 6);
    TCHECK(s_erase_sh_end2 == "hello");
    TCHECK(s_erase_sh_end == "hello world");

           // --- erase() on shared string, erasing entire content ---
    lString8 s_erase_sh_all {"abc"};
    lString8 s_erase_sh_all2 = s_erase_sh_all;
    s_erase_sh_all2.erase(0, 3);
    TCHECK(s_erase_sh_all2.empty());
    TCHECK(s_erase_sh_all == "abc");

           // --- append(const char*) on empty string ---
    lString8 s_app_empty;
    s_app_empty.append("hello");
    TCHECK(s_app_empty == "hello");
    TCHECK(s_app_empty.length() == 5);

           // --- append(const char*) on non-empty owned string ---
    lString8 s_app_own {"hello"};
    s_app_own.append(" world");
    TCHECK(s_app_own == "hello world");
    TCHECK(s_app_own.length() == 11);

           // --- append(const char*) with nullptr ---
    lString8 s_app_null {"test"};
    s_app_null.append(nullptr);
    TCHECK(s_app_null == "test");

           // --- append(const char*) with empty string ---
    lString8 s_app_emptystr {"test"};
    s_app_emptystr.append("");
    TCHECK(s_app_emptystr == "test");

           // --- append(const char*, count) on empty string ---
    lString8 s_app_fc_empty;
    s_app_fc_empty.append("hello world", 5);
    TCHECK(s_app_fc_empty == "hello");
    TCHECK(s_app_fc_empty.length() == 5);

           // --- append(const char*, count) with zero count ---
    lString8 s_app_fc_zero {"test"};
    s_app_fc_zero.append("ignored", 0);
    TCHECK(s_app_fc_zero == "test");

           // --- append(const char*, count) on shared string (COW) ---
    lString8 s_app_fc_shared {"base"};
    lString8 s_app_fc_copy = s_app_fc_shared;
    s_app_fc_copy.append(" extended", 9);
    TCHECK(s_app_fc_copy == "base extended");
    TCHECK(s_app_fc_copy.length() == 13);
    TCHECK(s_app_fc_shared == "base");

           // --- append(const char*, count) when capacity is insufficient ---
    lString8 s_app_fc_cap {"short"};
    s_app_fc_cap.reserve(5);  // ensure small capacity
    s_app_fc_cap.append(" much longer string", 19);
    TCHECK(s_app_fc_cap == "short much longer string");
    TCHECK(s_app_fc_cap.length() == 24);

           // --- append(const char*) chaining ---
    lString8 s_app_chain;
    s_app_chain.append("a");
    s_app_chain.append("b");
    s_app_chain.append("c");
    TCHECK(s_app_chain == "abc");

           // --- append(const char*, count) self-append ---
    lString8 s_app_self {"ab"};
    s_app_self.append(s_app_self.c_str(), 2);
    TCHECK(s_app_self == "abab");

           // --- append(const string&) on empty string ---
    lString8 s_app_s_empty;
    lString8 s_app_s_src {"hello"};
    s_app_s_empty.append(s_app_s_src);
    TCHECK(s_app_s_empty == "hello");

           // --- append(const string&) on non-empty string ---
    lString8 s_app_s_own {"hello "};
    s_app_s_own.append(s_app_s_src);
    TCHECK(s_app_s_own == "hello hello");

           // --- append(const string&) with empty source ---
    lString8 s_app_s_empty_src;
    lString8 s_app_s_nochange {"test"};
    s_app_s_nochange.append(s_app_s_empty_src);
    TCHECK(s_app_s_nochange == "test");

           // --- append(const string&) self-append ---
    lString8 s_app_s_self {"ab"};
    s_app_s_self.append(s_app_s_self);
    TCHECK(s_app_s_self == "abab");

           // --- append(const string&) self-append multiple times ---
    lString8 s_app_s_multi {"x"};
    s_app_s_multi.append(s_app_s_multi);
    TCHECK(s_app_s_multi == "xx");
    s_app_s_multi.append(s_app_s_multi);
    TCHECK(s_app_s_multi == "xxxx");

           // --- append(const string&) with insufficient capacity ---
    lString8 s_app_s_cap {"short"};
    s_app_s_cap.reserve(5);
    lString8 s_app_s_long {" much longer string"};
    s_app_s_cap.append(s_app_s_long);
    TCHECK(s_app_s_cap == "short much longer string");

           // --- append(const string&, offset, count) normal ---
    lString8 s_app_sf_src {"hello world"};
    lString8 s_app_sf {"start "};
    s_app_sf.append(s_app_sf_src, 6, 5);
    TCHECK(s_app_sf == "start world");

           // --- append(const string&, offset, count) self-append ---
    lString8 s_app_sf_self {"abcd"};
    s_app_sf_self.append(s_app_sf_self, 0, 4);
    TCHECK(s_app_sf_self == "abcdabcd");

    // --- append(const string&, offset, count) self-append fragment ---
    lString8 s_app_sf_self2 {"abcd"};
    s_app_sf_self2.append(s_app_sf_self2, 1, 2);
    TCHECK(s_app_sf_self2 == "abcdbc");

    // --- append(const string&, offset, count) count exceeds remaining ---
    lString8 s_app_sf_clamp {"hello"};
    lString8 s_app_sf_clap {"start"};
    s_app_sf_clap.append(s_app_sf_clamp, 3, 100);
    TCHECK(s_app_sf_clap == "startlo");

    // --- append(const string&, offset, count) offset beyond length ---
    lString8 s_app_sf_off {"hello"};
    lString8 s_app_sf_off2 {"start"};
    s_app_sf_off2.append(s_app_sf_off, 100, 5);
    TCHECK(s_app_sf_off2 == "start");

    // --- append(const string&, offset, count) empty source ---
    lString8 s_app_sf_empty;
    lString8 s_app_sf_empty2 {"test"};
    s_app_sf_empty2.append(s_app_sf_empty, 0, 5);
    TCHECK(s_app_sf_empty2 == "test");

    // --- append(count, char) on empty string ---
    lString8 s_app_cc_empty;
    s_app_cc_empty.append(5, 'x');
    TCHECK(s_app_cc_empty == "xxxxx");
    TCHECK(s_app_cc_empty.length() == 5);

    // --- append(count, char) on owned string with capacity ---
    lString8 s_app_cc_cap {"ab"};
    s_app_cc_cap.reserve(10);
    s_app_cc_cap.append(3, 'c');
    TCHECK(s_app_cc_cap == "abccc");
    TCHECK(s_app_cc_cap.length() == 5);

    // --- append(count, char) on owned string without capacity ---
    lString8 s_app_cc_nocap {"ab"};
    s_app_cc_nocap.append(5, 'z');
    TCHECK(s_app_cc_nocap == "abzzzzz");
    TCHECK(s_app_cc_nocap.length() == 7);

    // --- append(count, char) on shared string ---
    lString8 s_app_cc_shared {"base"};
    lString8 s_app_cc_copy = s_app_cc_shared;
    s_app_cc_copy.append(3, '!');
    TCHECK(s_app_cc_copy == "base!!!");
    TCHECK(s_app_cc_shared == "base");

    // --- append(count, char) with zero count ---
    lString8 s_app_cc_zero {"test"};
    s_app_cc_zero.append(0, 'x');
    TCHECK(s_app_cc_zero == "test");

    // --- append(count, char) with null char ---
    lString8 s_app_cc_null {"ab"};
    s_app_cc_null.append(2, '\0');
    TCHECK(s_app_cc_null.length() == 4);
    TCHECK(s_app_cc_null[2] == 0);
    TCHECK(s_app_cc_null[3] == 0);
    TCHECK(s_app_cc_null.c_str()[0] == 'a');

    // --- insert(pos, char*, count) into empty string ---
    lString8 s_ins_empty;
    s_ins_empty.insert(0, "hello", 5);
    TCHECK(s_ins_empty == "hello");
    TCHECK(s_ins_empty.length() == 5);

    // --- insert(pos, char*, count) at beginning ---
    lString8 s_ins_beg {"world"};
    s_ins_beg.insert(0, "hello ", 6);
    TCHECK(s_ins_beg == "hello world");

    // --- insert(pos, char*, count) in middle ---
    lString8 s_ins_mid {"helloworld"};
    s_ins_mid.insert(5, " ", 1);
    TCHECK(s_ins_mid == "hello world");

    // --- insert(pos, char*, count) at end ---
    lString8 s_ins_end {"hello"};
    s_ins_end.insert(5, " world", 6);
    TCHECK(s_ins_end == "hello world");

    // --- insert(pos, char*, count) with pos beyond length ---
    lString8 s_ins_off {"abc"};
    s_ins_off.insert(100, "xyz", 3);
    TCHECK(s_ins_off == "abcxyz");

    // --- insert(pos, char*, count) with zero count ---
    lString8 s_ins_zero {"test"};
    s_ins_zero.insert(2, "ignored", 0);
    TCHECK(s_ins_zero == "test");

    // --- insert(pos, char*, count) on shared string ---
    lString8 s_ins_shared {"base"};
    lString8 s_ins_copy = s_ins_shared;
    s_ins_copy.insert(2, "XX", 2);
    TCHECK(s_ins_copy == "baXXse");
    TCHECK(s_ins_shared == "base");

    // --- insert(pos, char*, count) with insufficient capacity ---
    lString8 s_ins_cap {"ab"};
    s_ins_cap.reserve(2);
    s_ins_cap.insert(1, " much longer ", 13);
    TCHECK(s_ins_cap == "a much longer b");
    TCHECK(s_ins_cap.length() == 15);

    // --- insert(pos, char*) ---
    lString8 s_ins_cstr {"helloworld"};
    s_ins_cstr.insert(5, " ");
    TCHECK(s_ins_cstr == "hello world");

           // --- insert(pos, char*) with nullptr ---
    lString8 s_ins_null {"test"};
    s_ins_null.insert(2, nullptr);
    TCHECK(s_ins_null == "test");

           // --- insert(pos, char*) with empty string ---
    lString8 s_ins_emptystr {"test"};
    s_ins_emptystr.insert(2, "");
    TCHECK(s_ins_emptystr == "test");

           // --- insert(pos, char*, count) self-insert prefix ---
    lString8 s_ins_self {"abcd"};
    s_ins_self.insert(2, s_ins_self.c_str(), 2);
    TCHECK(s_ins_self == "ababcd");

           // --- insert(pos, string&) ---
    lString8 s_ins_str {"helloworld"};
    lString8 s_ins_str_src {" "};
    s_ins_str.insert(5, s_ins_str_src);
    TCHECK(s_ins_str == "hello world");

           // --- insert(pos, string&) with empty source ---
    lString8 s_ins_str_empty {"test"};
    lString8 s_ins_str_empty_src;
    s_ins_str_empty.insert(2, s_ins_str_empty_src);
    TCHECK(s_ins_str_empty == "test");

           // --- insert(pos, string&) self-insert ---
    lString8 s_ins_str_self {"ab"};
    s_ins_str_self.insert(1, s_ins_str_self);
    TCHECK(s_ins_str_self == "aabb");

           // --- insert(pos, count, char) on empty string ---
    lString8 s_ins_cc_empty;
    s_ins_cc_empty.insert(0, 5, 'x');
    TCHECK(s_ins_cc_empty == "xxxxx");

           // --- insert(pos, count, char) in middle ---
    lString8 s_ins_cc_mid {"ab"};
    s_ins_cc_mid.insert(1, 3, '-');
    TCHECK(s_ins_cc_mid == "a---b");

           // --- insert(pos, count, char) at end ---
    lString8 s_ins_cc_end {"ab"};
    s_ins_cc_end.insert(2, 3, '!');
    TCHECK(s_ins_cc_end == "ab!!!");

           // --- insert(pos, count, char) pos beyond length ---
    lString8 s_ins_cc_off {"ab"};
    s_ins_cc_off.insert(100, 2, 'z');
    TCHECK(s_ins_cc_off == "abzz");

           // --- insert(pos, count, char) zero count ---
    lString8 s_ins_cc_zero {"test"};
    s_ins_cc_zero.insert(2, 0, 'x');
    TCHECK(s_ins_cc_zero == "test");

           // --- insert(pos, count, char) on shared string ---
    lString8 s_ins_cc_shared {"base"};
    lString8 s_ins_cc_copy = s_ins_cc_shared;
    s_ins_cc_copy.insert(2, 2, 'X');
    TCHECK(s_ins_cc_copy == "baXXse");
    TCHECK(s_ins_cc_shared == "base");

           // --- insert(pos, count, char) with insufficient capacity ---
    lString8 s_ins_cc_cap {"ab"};
    s_ins_cc_cap.reserve(2);
    s_ins_cc_cap.insert(1, 5, '-');
    TCHECK(s_ins_cc_cap == "a-----b");

    // --- pack() on string with excess capacity ---
    lString8 s_pack {"hello"};
    s_pack.reserve(1000);
    TCHECK(s_pack.capacity() >= 1000);
    s_pack.pack();
    TCHECK(s_pack == "hello");
    TCHECK(s_pack.length() == 5);
    TCHECK(s_pack.capacity() < 1000);

    // --- pack() on string with no excess capacity ---
    lString8 s_pack_exact {"exact"};
    size_t cap_before2 = s_pack_exact.capacity();
    s_pack_exact.pack();
    TCHECK(s_pack_exact == "exact");
    TCHECK(s_pack_exact.capacity() == cap_before2);

    // --- pack() on empty string ---
    lString8 s_pack_empty;
    s_pack_empty.pack();
    TCHECK(s_pack_empty.empty());

    // --- pack() on shared string ---
    lString8 s_pack_shared {"shared"};
    lString8 s_pack_copy = s_pack_shared;
    s_pack_copy.reserve(100);
    s_pack_copy.pack();
    TCHECK(s_pack_copy == "shared");
    TCHECK(s_pack_copy.capacity() < 100);
    TCHECK(s_pack_shared == "shared");

           // --- replace(pos, n, char*, count) normal ---
    lString8 s_rep1 {"hello world"};
    s_rep1.replace(6, 5, "earth", 5);
    TCHECK(s_rep1 == "hello earth");

           // --- replace(pos, n, char*, count) longer replacement ---
    lString8 s_rep2 {"abc"};
    s_rep2.replace(1, 1, "XYZ", 3);
    TCHECK(s_rep2 == "aXYZc");
    TCHECK(s_rep2.length() == 5);

           // --- replace(pos, n, char*, count) shorter replacement ---
    lString8 s_rep3 {"hello world"};
    s_rep3.replace(6, 5, "there", 5);
    TCHECK(s_rep3 == "hello there");

           // --- replace(pos, n, char*, count) n=0 (insert) ---
    lString8 s_rep4 {"ab"};
    s_rep4.replace(1, 0, "X", 1);
    TCHECK(s_rep4 == "aXb");

           // --- replace(pos, n, char*, count) pos beyond length ---
    lString8 s_rep5 {"ab"};
    s_rep5.replace(100, 5, "X", 1);
    TCHECK(s_rep5 == "abX");

           // --- replace(pos, n, char*, count) n exceeds remaining ---
    lString8 s_rep6 {"hello"};
    s_rep6.replace(3, 100, "X", 1);
    TCHECK(s_rep6 == "helX");

           // --- replace(pos, n, char*, count) on shared string ---
    lString8 s_rep7 {"shared"};
    lString8 s_rep7_copy = s_rep7;
    s_rep7_copy.replace(0, 6, "new", 3);
    TCHECK(s_rep7_copy == "new");
    TCHECK(s_rep7 == "shared");

           // --- replace(pos, n, char*) ---
    lString8 s_rep8 {"hello world"};
    s_rep8.replace(6, 5, "earth");
    TCHECK(s_rep8 == "hello earth");

           // --- replace(pos, n, char*) with nullptr ---
    lString8 s_rep9 {"test"};
    s_rep9.replace(1, 2, nullptr);
    TCHECK(s_rep9 == "test");

           // --- replace(pos, n, string&) ---
    lString8 s_rep10 {"hello world"};
    lString8 s_rep10_src {"earth"};
    s_rep10.replace(6, 5, s_rep10_src);
    TCHECK(s_rep10 == "hello earth");

           // --- replace(pos, n, string&) self-replace ---
    lString8 s_rep11 {"abcde"};
    s_rep11.replace(1, 3, s_rep11);
    TCHECK(s_rep11 == "aabcdee");

           // --- replace(pos, n, string&, offset, count) ---
    lString8 s_rep12 {"hello world"};
    lString8 s_rep12_src {"abcdefgh"};
    s_rep12.replace(6, 5, s_rep12_src, 2, 3);
    TCHECK(s_rep12 == "hello cde");

           // --- replace(pos, n, string&, offset, count) offset beyond ---
    lString8 s_rep13 {"test"};
    lString8 s_rep13_src {"abc"};
    s_rep13.replace(1, 2, s_rep13_src, 100, 5);
    TCHECK(s_rep13 == "test");

           // --- replace(pos, n, count, char) ---
    lString8 s_rep14 {"hello world"};
    s_rep14.replace(6, 5, 5, 'X');
    TCHECK(s_rep14 == "hello XXXXX");

           // --- replace(pos, n, count, char) longer ---
    lString8 s_rep15 {"abc"};
    s_rep15.replace(1, 1, 3, 'X');
    TCHECK(s_rep15 == "aXXXc");

           // --- replace(char, char) ---
    lString8 s_rep16 {"hello world"};
    s_rep16.replace('l', 'L');
    TCHECK(s_rep16 == "heLLo worLd");

           // --- replace(char, char) no match ---
    lString8 s_rep17 {"hello"};
    s_rep17.replace('z', 'Z');
    TCHECK(s_rep17 == "hello");

           // --- replace(char, char) on shared string ---
    lString8 s_rep18 {"abcabc"};
    lString8 s_rep18_copy = s_rep18;
    s_rep18_copy.replace('a', 'X');
    TCHECK(s_rep18_copy == "XbcXbc");
    TCHECK(s_rep18 == "abcabc");

           // --- replace(char, char) empty string ---
    lString8 s_rep19;
    s_rep19.replace('a', 'b');
    TCHECK(s_rep19.empty());

           // --- compare(pos, n, string&) ---
    lString8 s_cmp1 {"hello world"};
    lString8 s_cmp1_src {"world"};
    TCHECK(s_cmp1.compare(6, 5, s_cmp1_src) == 0);
    TCHECK(s_cmp1.compare(6, 5, lString8{"hello"}) > 0);  // "world" > "hello"
    TCHECK(s_cmp1.compare(6, 5, lString8{"zebra"}) < 0);  // "world" < "zebra"

           // --- compare(pos, n, string&) n exceeds bounds ---
    TCHECK(s_cmp1.compare(6, 100, s_cmp1_src) == 0);

           // --- compare(pos, n, string&) pos beyond length ---
    TCHECK(s_cmp1.compare(100, 5, lString8{"x"}) < 0);  // "" < "x"

           // --- compare(pos, n, string&, pos2, n2) ---
    lString8 s_cmp2 {"abcdef"};
    lString8 s_cmp2_src {"xyzbcdefg"};
    TCHECK(s_cmp2.compare(1, 4, s_cmp2_src, 3, 4) == 0);  // "bcde" == "bcde"
    TCHECK(s_cmp2.compare(1, 4, s_cmp2_src, 3, 3) > 0);   // "bcde" > "bcd"
    TCHECK(s_cmp2.compare(1, 3, s_cmp2_src, 3, 4) < 0);   // "bcd" < "bcde"

           // --- compare(pos, n, string&, pos2, n2) pos2 beyond ---
    TCHECK(s_cmp2.compare(1, 4, s_cmp2_src, 100, 5) > 0);  // "bcde" > ""

           // --- compare(pos, n, char*, n2) ---
    TCHECK(s_cmp2.compare(1, 4, "bcde", 4) == 0);
    TCHECK(s_cmp2.compare(1, 4, "bcd", 3) > 0);
    TCHECK(s_cmp2.compare(1, 3, "bcde", 4) < 0);

           // --- compare(pos, n, char*, n2) nullptr ---
    TCHECK(s_cmp2.compare(1, 4, nullptr, 0) > 0);  // "bcde" > ""

           // --- compare(pos, n, char*) ---
    TCHECK(s_cmp2.compare(1, 4, "bcde") == 0);
    TCHECK(s_cmp2.compare(1, 4, nullptr) > 0);
    TCHECK(s_cmp2.compare(1, 4, "") > 0);

           // --- compare(pos, n, char*) n exceeds bounds ---
    TCHECK(s_cmp2.compare(1, 100, "bcde") > 0);  // "bcdef" > "bcde"

           // --- compare(pos, n, char*) pos beyond length ---
    TCHECK(s_cmp2.compare(100, 5, "x") < 0);

           // --- compare(const char*) with nullptr ---
    lString8 s_cmp3 {"test"};
    TCHECK(s_cmp3.compare(nullptr) > 0);

           // --- compare(const char*) with empty ---
    TCHECK(s_cmp3.compare("") > 0);

           // --- compare(const char*) equal ---
    TCHECK(s_cmp3.compare("test") == 0);

           // --- compare(const char*) less/greater ---
    TCHECK(s_cmp3.compare("zebra") < 0);
    TCHECK(s_cmp3.compare("abc") > 0);

           // --- pos(char) ---
    lString8 s_pos1 {"hello world"};
    TCHECK(s_pos1.pos('o') == 4);
    TCHECK(s_pos1.pos('z') == lString8::npos);
    TCHECK(s_pos1.pos('h') == 0);
    TCHECK(s_pos1.pos('d') == 10);

           // --- pos(char) on empty string ---
    lString8 s_pos_empty;
    TCHECK(s_pos_empty.pos('a') == lString8::npos);

           // --- pos(char, start) ---
    TCHECK(s_pos1.pos('o', 5) == 7);
    TCHECK(s_pos1.pos('o', 8) == lString8::npos);
    TCHECK(s_pos1.pos('o', 4) == 4);
    TCHECK(s_pos1.pos('o', 100) == lString8::npos);

           // --- pos(string) ---
    lString8 s_pos2 {"hello world hello"};
    lString8 s_pos2_pat {"hello"};
    TCHECK(s_pos2.pos(s_pos2_pat) == 0);
    TCHECK(s_pos2.pos(lString8{"world"}) == 6);
    TCHECK(s_pos2.pos(lString8{"xyz"}) == lString8::npos);

           // --- pos(string) empty pattern ---
    TCHECK(s_pos2.pos(lString8{}) == lString8::npos);

           // --- pos(string) empty source ---
    lString8 s_pos_empty_src;
    TCHECK(s_pos_empty_src.pos(s_pos2_pat) == lString8::npos);

           // --- pos(string, start) ---
    TCHECK(s_pos2.pos(s_pos2_pat, 1) == 12);
    TCHECK(s_pos2.pos(s_pos2_pat, 13) == lString8::npos);
    TCHECK(s_pos2.pos(s_pos2_pat, 12) == 12);

           // --- pos(string) at exact end ---
    lString8 s_pos_end {"abcde"};
    TCHECK(s_pos_end.pos(lString8{"e"}) == 4);
    TCHECK(s_pos_end.pos(lString8{"de"}) == 3);
    TCHECK(s_pos_end.pos(lString8{"cde"}) == 2);
    TCHECK(s_pos_end.pos(lString8{"abcde"}) == 0);

           // --- pos(c-str) ---
    TCHECK(s_pos1.pos("world") == 6);
    TCHECK(s_pos1.pos("xyz") == lString8::npos);
    TCHECK(s_pos1.pos(nullptr) == lString8::npos);
    TCHECK(s_pos1.pos("") == lString8::npos);

           // --- pos(c-str, start) ---
    TCHECK(s_pos1.pos("world", 0) == 6);
    TCHECK(s_pos1.pos("world", 7) == lString8::npos);
    TCHECK(s_pos1.pos("world", 6) == 6);
    TCHECK(s_pos1.pos("world", 100) == lString8::npos);

           // --- pos(c-str) at exact end ---
    TCHECK(s_pos_end.pos("e") == 4);
    TCHECK(s_pos_end.pos("de") == 3);
    TCHECK(s_pos_end.pos("cde") == 2);
    TCHECK(s_pos_end.pos("abcde") == 0);

           // --- rpos(c-str) ---
    lString8 s_rpos1 {"hello world hello"};
    TCHECK(s_rpos1.rpos("hello") == 12);
    TCHECK(s_rpos1.rpos("world") == 6);
    TCHECK(s_rpos1.rpos("xyz") == lString8::npos);

           // --- rpos(c-str) single char ---
    TCHECK(s_rpos1.rpos("o") == 16);
    TCHECK(s_rpos1.rpos("h") == 12);
    TCHECK(s_rpos1.rpos("z") == lString8::npos);

           // --- rpos(c-str) at beginning ---
    TCHECK(s_rpos1.rpos("hello world") == 0);

           // --- rpos(c-str) entire string ---
    TCHECK(s_rpos1.rpos("hello world hello") == 0);

           // --- rpos(c-str) longer than string ---
    TCHECK(s_rpos1.rpos("hello world hello world") == lString8::npos);

           // --- rpos(c-str) nullptr ---
    TCHECK(s_rpos1.rpos(nullptr) == lString8::npos);

           // --- rpos(c-str) empty ---
    TCHECK(s_rpos1.rpos("") == lString8::npos);

           // --- rpos(c-str) empty string ---
    lString8 s_rpos_empty;
    TCHECK(s_rpos_empty.rpos("test") == lString8::npos);

           // --- startsWith(c-str) ---
    lString8 s_sw1 {"hello world"};
    TCHECK(s_sw1.startsWith("hello"));
    TCHECK(s_sw1.startsWith("hello world"));
    TCHECK(!s_sw1.startsWith("world"));
    TCHECK(!s_sw1.startsWith("hello world!"));
    TCHECK(!s_sw1.startsWith(""));
    TCHECK(!s_sw1.startsWith(nullptr));

           // --- startsWith(c-str, count) ---
    TCHECK(s_sw1.startsWith("hel", 3));
    TCHECK(s_sw1.startsWith("hello", 5));
    TCHECK(!s_sw1.startsWith("help", 4));
    TCHECK(!s_sw1.startsWith("hello world!", 12));

           // --- startsWith(string) ---
    lString8 s_sw2 {"hello"};
    TCHECK(s_sw1.startsWith(s_sw2));
    TCHECK(!s_sw2.startsWith(s_sw1));

           // --- startsWith(string) empty source ---
    lString8 s_sw_empty;
    TCHECK(!s_sw_empty.startsWith(s_sw2));
    TCHECK(!s_sw_empty.startsWith("hello"));

           // --- startsWith(string) empty pattern ---
    lString8 s_sw_empty_pat;
    TCHECK(!s_sw1.startsWith(s_sw_empty_pat));

           // --- endsWith(c-str) ---
    lString8 s_ew1 {"hello world"};
    TCHECK(s_ew1.endsWith("world"));
    TCHECK(s_ew1.endsWith("hello world"));
    TCHECK(!s_ew1.endsWith("hello"));
    TCHECK(!s_ew1.endsWith("hello world!"));
    TCHECK(!s_ew1.endsWith(""));
    TCHECK(!s_ew1.endsWith(nullptr));

           // --- endsWith(c-str, count) ---
    TCHECK(s_ew1.endsWith("orld", 4));
    TCHECK(s_ew1.endsWith("world", 5));
    TCHECK(!s_ew1.endsWith("word", 4));
    TCHECK(!s_ew1.endsWith("hello world!", 12));

           // --- endsWith(string) ---
    lString8 s_ew2 {"world"};
    TCHECK(s_ew1.endsWith(s_ew2));
    TCHECK(!s_ew2.endsWith(s_ew1));

           // --- endsWith(string) empty source ---
    lString8 s_ew_empty;
    TCHECK(!s_ew_empty.endsWith(s_ew2));
    TCHECK(!s_ew_empty.endsWith("world"));

           // --- endsWith(string) empty pattern ---
    lString8 s_ew_empty_pat;
    TCHECK(!s_ew1.endsWith(s_ew_empty_pat));

           // --- substr(pos, count) ---
    lString8 s_sub1 {"hello world"};
    TCHECK(s_sub1.substr(0, 5) == "hello");
    TCHECK(s_sub1.substr(6, 5) == "world");
    TCHECK(s_sub1.substr(0, 11) == "hello world");
    TCHECK(s_sub1.substr(5, 1) == " ");
    TCHECK(s_sub1.substr(10, 1) == "d");

           // --- substr(pos, count) count exceeds remaining ---
    TCHECK(s_sub1.substr(6, 100) == "world");

           // --- substr(pos, count) pos beyond length ---
    TCHECK(s_sub1.substr(100, 5) == "");

           // --- substr(pos, count) pos at end ---
    TCHECK(s_sub1.substr(11, 5) == "");

           // --- substr(pos) default count=npos ---
    TCHECK(s_sub1.substr(6) == "world");
    TCHECK(s_sub1.substr(0) == "hello world");

           // --- substr(pos, count) empty string ---
    lString8 s_sub_empty;
    TCHECK(s_sub_empty.substr(0, 5) == "");
    TCHECK(s_sub_empty.substr(0) == "");

           // --- empty_str ---
    lString8 s_empty_ref = lString8::empty_str;
    TCHECK(s_empty_ref.empty());
    TCHECK(s_empty_ref.length() == 0);
    TCHECK(s_empty_ref == "");

           // --- operator!() ---
    lString8 s_not1 {"hello"};
    lString8 s_not2;
    TCHECK(!s_not2);
    TCHECK(!(!s_not1));

           // --- swap() ---
    lString8 s_swap1 {"first"};
    lString8 s_swap2 {"second"};
    s_swap1.swap(s_swap2);
    TCHECK(s_swap1 == "second");
    TCHECK(s_swap2 == "first");

           // --- swap() with empty ---
    lString8 s_swap3 {"nonempty"};
    lString8 s_swap4;
    s_swap3.swap(s_swap4);
    TCHECK(s_swap3.empty());
    TCHECK(s_swap4 == "nonempty");

           // --- swap() self ---
    lString8 s_swap5 {"self"};
    s_swap5.swap(s_swap5);
    TCHECK(s_swap5 == "self");
}

void test_writable_refs_lString8() {
    lString8 s1 {"Original string"};
    lString8 s1_copy = s1;
    TCHECK(s1 == s1_copy);
    auto& s1_wr = s1.writableRef(100);
    TCHECK(s1_wr.length() == s1_copy.length());

}

void test_lstring2() {
    printf("New strings library tests\n");
    test_lstring2_chunks();
    DUMP_ALLOC_STATS("before test_lstring8()")
    test_lstring8();
    DUMP_ALLOC_STATS("after test_lstring8()")
    test_writable_refs_lString8();

    if (test_errors == 0) {
        printf("New strings library tests completed successfully\n");
    } else {
        printf("***\nNEW STRINGS LIBRARY TESTS COMPLETED WITH %d ERRORS\n***\n", test_errors);
    }
    printf("New strings library completed\n");
}

}
