#include "lvstring2test.h"
#include <stdio.h>

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
}

void test_lstring2() {
    printf("New strings library tests\n");
    test_lstring2_chunks();
    DUMP_ALLOC_STATS("before test_lstring8()")
    test_lstring8();
    DUMP_ALLOC_STATS("after test_lstring8()")

    if (test_errors == 0) {
        printf("New strings library tests completed successfully\n");
    } else {
        printf("***\nNEW STRINGS LIBRARY TESTS COMPLETED WITH %d ERRORS\n***\n", test_errors);
    }
    printf("New strings library completed\n");
}

}
