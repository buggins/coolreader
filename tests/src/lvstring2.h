#ifndef LVSTRING2_H
#define LVSTRING2_H

/**
 * Copy-on-write reference counting strings.
 *
 * Optimized for low footprint and high performance.
 * Thread-safe if instantiated with std::atomic_int as refcounter_type.
 *
 * Designed to be a drop-in replacement for old lString8/lString16/lString32 in coolreader core.
 *
 * Empty string is internally represented by `nullptr`.
 *
 * Length, capacity and refcount are located in signle block alongside with character data, occupying 12 bytes for 4-byte size_type and refcounter_type.
 * Space for additional zero-termination character is always allocated in addition to capacity() value.
 *
 * To minimize refcount checks, use writable reference to modify string content.
 *
 * Interface is partially compatible with std::string.
 */

#include "lvtypes.h"
#include <atomic>
#include <stdlib.h>
#include <cstdlib>
#include <cstring>

#define DEBUG_TRACK_LSTRING2_ALLOC

/// helper functions from lvstring
void lStr_uppercase( lChar32 * str, int len );
void lStr_uppercase( lChar16 * str, int len );
void lStr_uppercase( lChar8 * str, int len );
void lStr_lowercase( lChar32 * str, int len );
void lStr_lowercase( lChar16 * str, int len );
void lStr_lowercase( lChar8 * str, int len );
lUInt16 lGetCharProps( lChar32 ch );
bool isAlNum(lChar32 ch);


// returns 0..15 if c is hex digit, -1 otherwise
int hexDigit( int c );
// converts 0..15 to 0..f
char toHexDigit( int c );

namespace lv {

#ifdef DEBUG_TRACK_LSTRING2_ALLOC
struct lStringStats {
    int allocCount {0};
    int freeCount {0};
    int copyConstr {0};
    int moveConstr {0};
    int copyAssign {0};
    int moveAssign {0};
    void dump(const char * msg = "");
};
extern lStringStats ls_alloc_stats;

#define LS_COUNT_ALLOC ls_alloc_stats.allocCount++;
#define LS_COUNT_FREE ls_alloc_stats.freeCount++;
#define LS_COUNT_COPY_CONSTR ls_alloc_stats.copyConstr++;
#define LS_COUNT_MOVE_CONSTR ls_alloc_stats.moveConstr++;
#define LS_COUNT_COPY_ASSIGN ls_alloc_stats.copyAssign++;
#define LS_COUNT_MOVE_ASSIGN ls_alloc_stats.moveAssign++;

#else

#define LS_COUNT_ALLOC
#define LS_COUNT_FREE
#define LS_COUNT_COPY_CONSTR
#define LS_COUNT_MOVE_CONSTR
#define LS_COUNT_COPY_ASSIGN
#define LS_COUNT_MOVE_ASSIGN

#endif


// lv::fmt copy of fmt
namespace fmt {
    class decimal {
        lInt64 value;
      public:
        explicit decimal(lInt64 v) : value(v) { }
        lInt64 get() const { return value; }
    };

    class hex {
        lUInt64 value;
      public:
        explicit hex(lInt64 v) : value(v) { }
        lUInt64 get() const { return value; }
    };
}


// Helper functions

/// arbitrary char type strlen, supports nullptr arg
template<typename char_type, typename size_type>
inline size_type str_len(const char_type * s) {
    if (s == nullptr) {
        return 0;
    }
    size_type i = 0;
    while (s[i] != 0) {
        i++;
    }
    return i;
}

/// arbitrary char type strcmp, supports empty strings
template<typename char_type, typename size_type>
inline int str_cmp(const char_type * s1, size_type sz1, const char_type * s2, size_type sz2) {
    // support case when one of strings or both are empty
    if (sz1 == 0) {
        return sz2 > 0 ? -1 : 0;
    } else if (sz2 == 0) {
        return 1;
    }
    for (size_type i = 0; ; i++) {
        if (i >= sz1) {
            // equal or s2 is bigger
            return i < sz2 ? -1 : 0;
        } else if (i >= sz2) {
            // less
            return 1;
        }
        if (s1[i] < s2[i]) {
            return -1;
        }
        if (s1[i] > s2[i]) {
            return 1;
        }
        // char [i] is matching, continue
    }
}

/// arbitrary char type strcmp, both args are non-empty
template<typename char_type, typename size_type>
inline int str_cmp_nonempty(const char_type * s1, size_type sz1, const char_type * s2, size_type sz2) {
    // we are sure that sz1>0 && sz2>0
    for (size_type i = 0; ; i++) {
        if (i >= sz1) {
            // end of s1
            // equal or s2 is bigger
            return (i < sz2) ? -1 : 0;
        } else if (i >= sz2) {
            // s2 not yet ended
            return 1;
        }
        if (s1[i] < s2[i]) {
            return -1;
        }
        if (s1[i] > s2[i]) {
            return 1;
        }
        // char [i] is matching, continue
    }
}

/// arbitrary char type strcmp, both args are non-empty, s2 is zero-terminated
template<typename char_type, typename size_type>
inline int str_cmp_nonempty(const char_type * s1, size_type sz1, const char_type * s2) {
    // we are sure that sz1>0 && sz2>0
    for (size_type i = 0; ; i++) {
        if (i >= sz1) {
            // end of s1
            // equal or s2 is bigger
            return s2[i] ? -1 : 0;
        } else if (!s2[i]) {
            // end of s2
            // bigger
            return 1;
        }
        if (s1[i] < s2[i]) {
            return -1;
        }
        if (s1[i] > s2[i]) {
            return 1;
        }
        // char [i] is matching, continue
    }
}

/// arbitrary char type strcmp, both args are non-empty, zero-terminated
template<typename char_type, typename size_type>
inline int str_cmp_nonempty(const char_type * s1, const char_type * s2) {
    // we are sure that sz1>0 && sz2>0
    for (size_type i = 0; ; i++) {
        if (!s1[i]) {
            // end of s1
            // equal or s2 is bigger
            return s2[i] ? -1 : 0;
        } else if (!s2[i]) {
            // end of s2
            // bigger
            return 1;
        }
        if (s1[i] < s2[i]) {
            return -1;
        }
        if (s1[i] > s2[i]) {
            return 1;
        }
        // char [i] is matching, continue
    }
}

// String data buffer with ref count

// forward declaration of string readonly class
template<typename char_type, typename refcounter_type>
class string_ro;
// forward declaration of string writable ref class
template<typename char_type, typename refcounter_type>
class string_wr;
// forward declaration of string class
template<typename char_type, typename refcounter_type>
class string;

template <typename char_type, typename size_type, typename refcounter_type = std::atomic_int>
struct lstring_chunk_t {

    template <typename, typename> friend class string_ro;
    template <typename, typename> friend class string;
    template <typename, typename> friend class string_wr;

    friend void test_lstring2_chunks();
    /// chunk allocation alignment in bytes
    static constexpr size_type alloc_align_bytes = 16;
    /// minimum reserved characters count to have allocated size (alloc_align_bytes) bytes
    static constexpr size_type min_size = static_cast<size_type>((alloc_align_bytes - sizeof(lstring_chunk_t)) / sizeof(char_type)) - 1;
    /// character count increment to keep block size aligned
    static constexpr size_type size_align = static_cast<size_type>(alloc_align_bytes / sizeof(char_type));

public:

    //const char_type * data() const { return &buf[0]; }

    /// align reserved size in characters of block to have byte alignment alloc_align_bytes, size does not include trailing 0 character
    inline static size_type alignSize(size_type sz) noexcept {
        if (sz <= min_size)
            return min_size;
        return min_size + (((sz - min_size) + size_align - 1) & (0 - size_align));
    }

    /// get reference counter value
    size_type getRefCount() const noexcept {
        if constexpr (std::is_same_v<refcounter_type, size_type>) {
            // simple refcount
            return refCount;
        } else {
            // atomic
            return refCount.load(std::memory_order_relaxed);
        }
    }

    /// returns true if ref counter value is 1
    bool isOwn() const noexcept {
        if constexpr (std::is_same_v<refcounter_type, size_type>) {
            // simple refcount
            return refCount == 1;
        } else {
            // atomic
            return refCount.load(std::memory_order_relaxed) == 1;
        }
    }

    /// returns true if ref counter value is 1
    bool isShared() const noexcept {
        if constexpr (std::is_same_v<refcounter_type, size_type>) {
            // simple refcount
            return refCount > 1;
        } else {
            // atomic
            return refCount.load(std::memory_order_relaxed) > 1;
        }
    }

    /// 1. Hook for incrementing the counter (compatible with boost::intrusive_ptr)
    friend void intrusive_ptr_add_ref(const lstring_chunk_t* p) noexcept {
        if constexpr (std::is_same_v<refcounter_type, size_type>) {
            // simple refcount
            p->refCount++;
        } else {
            p->refCount.fetch_add(1, std::memory_order_relaxed);
        }
    }

    /// 1. Hook for incrementing the counter (compatible with boost::intrusive_ptr)
    friend void intrusive_ptr_add_ref_checknull(const lstring_chunk_t* p) noexcept {
        if (p) {
            if constexpr (std::is_same_v<refcounter_type, size_type>) {
                // simple refcount
                p->refCount++;
            } else {
                p->refCount.fetch_add(1, std::memory_order_relaxed);
            }
        }
    }

    void testAddRef() {
        refCount.fetch_add(1, std::memory_order_relaxed);
    }

    void testReleaseRef() {
        refCount.fetch_sub(1, std::memory_order_acq_rel);
    }

    /// 2. Hook for decrementing and deleting the object (compatible with boost::intrusive_ptr)
    friend void intrusive_ptr_release(const lstring_chunk_t* p) noexcept {
        if constexpr (std::is_same_v<refcounter_type, size_type>) {
            if ((p->refCount--) == 1) {
                lstring_chunk_t::free(p);
            }
        } else {
            if (p->refCount.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                lstring_chunk_t::free(p);
            }
        }
    }

    /// chunk allocation function: create string buffer with reserved sz characters + zero termination char, ref counter 1
    static lstring_chunk_t * alloc(size_type sz) noexcept {
        LS_COUNT_ALLOC
        //lstring_chunk_t * res = static_cast<lstring_chunk_t *>( ::malloc(sizeof(lstring_chunk_t) + sizeof(char_type) * (sz + 1)) );
        //lstring_chunk_t * res = static_cast<lstring_chunk_t *>( ::malloc(sizeof(lstring_chunk_t) + sizeof(char_type) * (sz + 1)) );
        sz = alignSize(sz);
        size_t allocBytes = (sizeof(lstring_chunk_t) + sizeof(char_type) * (sz + 1));
        lstring_chunk_t * res = static_cast<lstring_chunk_t *>( std::aligned_alloc(alloc_align_bytes, allocBytes) );
        res->size = sz;
        res->len = 0;
        res->refCount = 1;
        return res;
    }

    /// chunk allocation function: create string buffer initialized with count chars from string s with reserved sz characters + zero termination char, ref counter 1
    static lstring_chunk_t * alloc(const char_type * s, size_type count, size_type sz = 0) noexcept {
        LS_COUNT_ALLOC
        if (sz < count)
            sz = count;
        sz = alignSize(sz);
        size_t allocBytes = (sizeof(lstring_chunk_t) + sizeof(char_type) * (sz + 1));
        lstring_chunk_t * res = static_cast<lstring_chunk_t *>( std::aligned_alloc(alloc_align_bytes, allocBytes) );
        std::memcpy(res->buf, s, count * sizeof(char_type));
        res->size = sz;
        res->len = count;
        res->refCount = 1;
        return res;
    }

    /// chunk allocation function: create duplicate of this string buffer with ref counter 1, does not release this buffer
    lstring_chunk_t * duplicate() noexcept {
        LS_COUNT_ALLOC
        size_t allocBytes = (sizeof(lstring_chunk_t) + sizeof(char_type) * (size + 1));
        lstring_chunk_t * res = static_cast<lstring_chunk_t *>( std::aligned_alloc(alloc_align_bytes, allocBytes) );
        std::memcpy(res->buf, buf, len * sizeof(char_type));
        res->size = size;
        res->len = len;
        res->refCount = 1;
        return res;
    }

    /// chunk allocation function: create duplicate of this string buffer with capacity newSize and ref counter 1, does not release this buffer
    lstring_chunk_t * duplicate(size_type newSize) noexcept {
        LS_COUNT_ALLOC
        size_type sz = newSize;
        size_type count = len > sz ? sz : len;
        size_t allocBytes = (sizeof(lstring_chunk_t) + sizeof(char_type) * (sz + 1));
        lstring_chunk_t * res = static_cast<lstring_chunk_t *>( std::aligned_alloc(alloc_align_bytes, allocBytes) );
        std::memcpy(res->buf, buf, count * sizeof(char_type));
        res->size = sz;
        res->len = count;
        res->refCount = 1;
        return res;
    }

    /// chunk allocation function: create duplicate of this string buffer with minimum capacity to store the data and ref counter 1, and replace dest buffer with created copy
    /// Don't call on for shared destination buffer.
    /// if destination buffer exists:
    /// * if destination buffer has enough capacity to store this string -- just copy data
    /// * if destination buffer capacity is too low, free it (assuming dest buffer is not shared) and replace with a created duplicate.
    void duplicate_to_owned(lstring_chunk_t* &dest) noexcept {
        if (dest && dest->size >= len) {
            // destination has enough space for this string: copy content to destination buffer
            // assuming dest buffer is not shared!!!
            if (len)
                std::memcpy(dest->buf, buf, len * sizeof(char_type));
            dest->len = len;
        } else {
            LS_COUNT_ALLOC
            // allocate minimum available size enough for storing this data
            size_type sz = alignSize(len);
            size_t allocBytes = (sizeof(lstring_chunk_t) + sizeof(char_type) * (sz + 1));
            lstring_chunk_t * res = static_cast<lstring_chunk_t *>( std::aligned_alloc(alloc_align_bytes, allocBytes) );
            if (len)
                std::memcpy(res->buf, buf, len * sizeof(char_type));
            res->size = sz;
            res->len = len;
            res->refCount = 1;
            if (dest) {
                // assuming dest buffer is not shared!!! just free instead of release ref
                free(dest);
            }
            dest = res;
        }
    }

   /// chunk allocation function: create duplicate of this string buffer fragment to store the data and ref counter 1, and replace dest buffer with created copy
   /// Don't call on for shared destination buffer.
   /// if destination buffer exists:
   /// * if destination buffer has enough capacity to store this string -- just copy data
   /// * if destination buffer capacity is too low, free it (assuming dest buffer is not shared) and replace with a created duplicate.
    void duplicate_to_owned(lstring_chunk_t* &dest, size_type start, size_type count) noexcept {
        if (start >= len || !count) {
            // empty string assignment
            if (dest) {
                // just reset length
                dest->len = 0;
            }
            return;
        }
        if (start + count > len) {
            count = len - start;
        }
        if (dest && dest->size >= count) {
            // destination has enough space for this string: copy content to destination buffer
            // assuming dest buffer is not shared!!!
            std::memcpy(dest->buf, buf + start, count * sizeof(char_type));
            dest->len = count;
        } else {
            LS_COUNT_ALLOC

            // allocate minimum available size enough for storing this data
            size_type sz = alignSize(count);
            size_t allocBytes = (sizeof(lstring_chunk_t) + sizeof(char_type) * (sz + 1));
            lstring_chunk_t * res = static_cast<lstring_chunk_t *>( std::aligned_alloc(alloc_align_bytes, allocBytes) );
            // copy characters
            std::memcpy(res->buf, buf + start, count * sizeof(char_type));
            res->size = sz;
            res->len = count;
            res->refCount = 1;
            if (dest) {
                // assuming dest buffer is not shared!!! just free instead of release ref
                free(dest);
            }
            dest = res;
        }
    }

    /// only copy content of other buffer to this buffer
    /// Don't call for shared this!
    /// returns number of characters copied
    size_type copy_from(const lstring_chunk_t * p) noexcept {
        if (!p || !p->len) {
            // only reset buffer length if attempting to copy from empty buffer
            len = 0;
        } else {
            len = p->len;
            if (len > size)
                len = size;
            std::memcpy(buf, p->buf, len * sizeof(char_type));
        }
        return len;
    }

    /// free chunk memory
    static void free( const lstring_chunk_t * pChunk ) noexcept {
        LS_COUNT_FREE
        ::free(const_cast<lstring_chunk_t *>(pChunk));
    }

private:
    lstring_chunk_t() : size(1), len(0)
    {
        refCount = 1;
    }
    ~lstring_chunk_t() {};
    // layout for 4-byte size_type and refcounew
    /// 0: size (max capacity for chars) less 1
    size_type       size;     // 0 for free chunk
    /// 4: length (number of chars in array)
    size_type       len;      // count of chars in string
    /// 8: reference counter
    mutable refcounter_type refCount; // reference counter
    /// 12: string characters buffer of variable size
    char_type       buf[0];      // z-string
};

/// returns number of elements to convert unicode codepoint into char_type utf.
/// * for sizeof(char_type)==4 returns 1
/// * for sizeof(char_type)==2 returns number of utf16 codepoints(1 to 2)
/// * for sizeof(char_type)==2 returns number of utf8 codepoints(1 to 4)
template<typename char_type>
inline lInt32 utfCodePointSize(lChar32 c) noexcept {
    if constexpr(sizeof(char_type) == 1) {
        // calculate number of utf8 codepoints
        return (c < 0x80)    ? 1
            :  (c < 0x800)   ? 2
            :  (c < 0x10000) ? 3
            :                  4;
    } else if constexpr(sizeof(char_type) == 2) {
        // calculate number of utf16 codepoints
        return (c >= 0x10000) ? 2 : 1;
    } else {
        // assuming this is 4-byte type representing unicode codepoint
        return 1;
    }
}

//constexpr lChar32 UNICODE_END_OF_STREAM = 0xFFFFFFFF;

/// read one unicode point from utf stream from position s and advances position of s
/// * supports 1-byte (utf8), 2-byte (utf16) and 4-byte (utf32) inputs
/// * returns unicode codepoint value if it's decoded successfully
/// * returns invalid_char value if invalid data found in input buffer (advances pointer anyway)
/// * overlong sequence (e.g. 7-bit code encoded as 3 bytes) is not considered as invalid character
/// * returns end_of_stream value if reading position reaches end of buffer (pend)
template<typename char_type, lChar32 invalid_char = 0xFFFFFFFEu, lChar32 end_of_stream = 0xFFFFFFFFu>
inline lChar32 utfReadCodePoint(const char_type* &s, const char_type* pend) noexcept {
    if (s >= pend) {
        return end_of_stream;
    }
    lChar32 c0 = (*s++);
    if constexpr(sizeof(char_type) == 1) {
        // calculate number of utf8 codepoints
        if (c0 < 0x80) {
            // single-byte utf8 character
            return c0;
        }
        if ((c0 & 0xC0) == 0x80) {
            // byte 10xx_xxxx is unexpected begin of utf8 sequence, return invalid char
            return invalid_char;
        }
        if (s >= pend) {
            // end of stream reached while waiting for additional bytes of codepoint -- indicate wrong character
            // next call to this function will return end_of_stream because we reached end of stream
            return invalid_char;
        }
        lChar32 c1 = (*s++);
        if ((c1 & 0xC0) != 0x80) {
            // byte 10xx_xxxx is expected for all additional bytes of utf8 sequence
            return invalid_char;
        }
        if ((c0 & 0xE0) == 0xC0) {
            // 2-byte codepoint
            return ((c0 & 0x1F) << 6) | (c1 & 0x3f);
        }
        // need one more byte
        if (s >= pend) {
            // end of stream reached while waiting for additional bytes of codepoint -- indicate wrong character
            // next call to this function will return end_of_stream because we reached end of stream
            return invalid_char;
        }
        // read byte
        lChar32 c2 = (*s++);
        if ((c2 & 0xC0) != 0x80) {
            // byte 10xx_xxxx is expected for all additional bytes of utf8 sequence
            return invalid_char;
        }
        if ((c0 & 0xF0) == 0xE0) {
            // 3-byte codepoint
            c0 = ((c0 & 0x0F) << 12) | ((c1 & 0x3f) << 6) | (c2 & 0x3f);
            // validate value to be in valid unicode range
            return (c0 > 0x10FFFF || (c0 >= 0xD800 && c0 <= 0xDFFF)) ? invalid_char : c0;
        }
        // need one more byte
        if (s >= pend) {
            // end of stream reached while waiting for additional bytes of codepoint -- indicate wrong character
            // next call to this function will return end_of_stream because we reached end of stream
            return invalid_char;
        }
        // read byte
        lChar32 c3 = (*s++);
        if ((c3 & 0xC0) != 0x80) {
            // byte 10xx_xxxx is expected for all additional bytes of utf8 sequence
            return invalid_char;
        }
        if ((c0 & 0xF8) == 0xF0) {
            // 3-byte codepoint
            c0 = ((c0 & 0x07) << 18) | ((c1 & 0x3f) << 12) | ((c2 & 0x3f) << 6) | (c3 & 0x3f);
            // validate value to be in valid unicode range
            return (c0 > 0x10FFFF || (c0 >= 0xD800 && c0 <= 0xDFFF)) ? invalid_char : c0;
        }
        // incorrect utf8 sequence - 4-byte max length is expected
        return invalid_char;
    } else if constexpr(sizeof(char_type) == 2) {
        // calculate number of utf16 codepoints
        if (c0 >= 0xD800 && c0 <= 0xDFFF) {
            // c0 is either low or high part of surrogate pair
            if (c0 >= 0xDC00) {
                // found low part of surrogate pair - invalid position inside utf16 stream -- skip it
                return invalid_char;
            }
            // c0 is high part of surrogate pair, should be followed by low part in range (c0 >= 0xDC00 && c0 <= 0xDFFF)
            if (s >= pend) {
                // there must follow low part of surrogate pair, but end of stream reached
                // return invalid_char -- next call to this function will return end_of_stream
                return invalid_char;
            }
            // reading low part of surrogate pair
            lChar32 c1 = (*s++);
            if (c1 < 0xDC00 || c1 > 0xDFFF) {
                // range is out of low part of surrogate pair
                return invalid_char;
            }
            // c0 (high part of surrogate) must be in range 0xD800..0xDBFF
            // c1 (low part of surrogate) must be in range 0xDC00..0xDFFF
            return (((c0 & 0x3FF) << 10) | (c1 & 0x3FF)) + 0x10000;
        }
        // single-item character, not a surrogate pair return as is
        return c0;
    } else {
        // assuming this is 4-byte type representing unicode codepoint
        return (c0 > 0x10FFFF || (c0 >= 0xD800 && c0 <= 0xDFFF)) ? invalid_char: c0;
    }
}

/// Writes one unicode point to utf buffer `s` ending at `pend` and advances position of `s`
/// * supports 1-byte (utf8), 2-byte (utf16) and 4-byte (utf32) inputs
/// * returns unicode codepoint value if it's decoded successfully
/// * returns `ch' value if invalid data found in input buffer (advances pointer anyway)
/// * returns `invalid_char` value if codepoint passed on input is out of range (writing is skipped)
/// * returns `end_of_stream` value if buffer has not enough space
template<typename char_type, lChar32 invalid_char = 0xFFFFFFFEu, lChar32 end_of_stream = 0xFFFFFFFFu>
inline lChar32 utfWriteCodePoint(lChar32 ch, char_type* &s, const char_type* pend) noexcept {
    // don't write invalid character to output
    if constexpr(sizeof(char_type) == 1) {
        // encode as utf8
        int space = static_cast<int>( pend - s );
        if (ch < 0x80) {
            // single byte utf8 sequence
            if (space < 1) // no space in output buffer
                return end_of_stream;
            *s++ = static_cast<char_type>(ch);
        } else if (ch < 0x800) {
            // 2-byte utf8 sequence
            if (space < 2)
                return end_of_stream;
            s[0] = static_cast<char_type>(0xC0 | (ch >> 6));
            s[1] = static_cast<char_type>(0x80 | (ch & 0x3F));
            s += 2;
        } else if (ch < 0x10000) {
            // 3-byte utf8 sequence
            if (ch >= 0xD800 && ch <= 0xDFFF)
                return invalid_char;
            if (space < 3) // no space in output buffer
                return end_of_stream;
            s[0] = static_cast<char_type>(0xE0 | (ch >> 12));
            s[1] = static_cast<char_type>(0x80 | ((ch >> 6) & 0x3F));
            s[2] = static_cast<char_type>(0x80 | ((ch) & 0x3F));
            s += 3;
        } else {
            // 4-byte utf8 sequence
            if (ch > 0x10FFFF || (ch >= 0xD800 && ch <= 0xDFFF))
                return invalid_char;
            if (space < 4) // no space in output buffer
                return end_of_stream;
            s[0] = static_cast<char_type>(0xF0 | (ch >> 18));
            s[1] = static_cast<char_type>(0x80 | ((ch >> 12) & 0x3F));
            s[2] = static_cast<char_type>(0x80 | ((ch >> 6) & 0x3F));
            s[3] = static_cast<char_type>(0x80 | ((ch) & 0x3F));
            s += 4;
        }
    } else if constexpr(sizeof(char_type) == 2) {
        // encode as utf16
        int space = static_cast<int>( pend - s );
        if (ch < 0x10000) {
            // single word
            if (ch >= 0xD800 && ch <= 0xDFFF)
                return invalid_char;
            if (space < 1) // no space in output buffer
                return end_of_stream;
            // write as is
            *s++ = static_cast<char_type>(ch);
        } else {
            // double word
            if (ch > 0x10FFFF)
                return invalid_char;
            if (space < 2) // no space in output buffer
                return end_of_stream;
            // write 2 utf16 codes: high, then low
            lChar32 c = ch - 0x10000;
            s[0] = static_cast<char_type>(0xD800 | ((c >> 10) & 0x3FF));
            s[1] = static_cast<char_type>(0xDC00 | (c & 0x3FF));
            s += 2;
        }
    } else {
        // encode as utf32
        if (ch > 0x10FFFF || (ch >= 0xD800 && ch <= 0xDFFF))
            return invalid_char;
        if (s >= pend) {
            return end_of_stream;
        }
        *s++ = ch;
    }
    return ch;
}

/// Calculate buffer size in dst_char_type items to transcode utf string from src_char_type to dst_char_type.
/// Assuming that invalid unicode codepoints will be filtered out during conversion.
template <typename src_char_type, typename dst_char_type, lChar32 invalid_char = 0xFFFFFFFEu, lChar32 end_of_stream = 0xFFFFFFFFu>
inline lUInt32 utfConvDestBufferSize(const src_char_type * pstart, const src_char_type * pend) {
    lUInt32 count = 0;
    for(;;) {
        // read next unicode codepoint
        lChar32 ch = utfReadCodePoint<src_char_type, invalid_char, end_of_stream>(pstart, pend);
        if (ch == end_of_stream) {
            return count;
        }
        if (ch != invalid_char) {
            // ignore invalid unicode codepoints
            count += utfCodePointSize<dst_char_type>(ch);
        }
    }
}

/// Convert utf string from src_char_type to dst_char_type.
/// Invalid unicode codepoints will be filtered out during conversion (Returns number of invalid characters skipped in errorCount.)
/// Returns number of dst_char_type utf elements written to destination buffer.
template <typename src_char_type, typename dst_char_type, lChar32 invalid_char = 0xFFFFFFFEu, lChar32 end_of_stream = 0xFFFFFFFFu>
inline lUInt32 utfConvert(const src_char_type * &src_start, const src_char_type * src_end, dst_char_type * &dst_start, const dst_char_type * dst_end, lUInt32 &errorCount) {
    errorCount = 0;
    const dst_char_type* keep_dst_start = dst_start;
    for(;;) {
        // read next unicode codepoint
        lChar32 ch = utfReadCodePoint<src_char_type, invalid_char, end_of_stream>(src_start, src_end);
        if (ch == end_of_stream) {
            return dst_start - keep_dst_start;
        }
        if (ch != invalid_char) {
            lChar32 res = utfWriteCodePoint<dst_char_type, invalid_char, end_of_stream>(ch, dst_start, dst_end);
            if (res == end_of_stream) {
                // no space left in dest buffer
                return dst_start - keep_dst_start;
            }
        } else {
            errorCount++;
        }
    }
}

/// counts characters in utf-encoded buffer
/// * supports 1-byte (utf8), 2-byte (utf16) and 4-byte (utf32) inputs
template<typename char_type, lChar32 invalid_char = 0xFFFFFFFEu, lChar32 end_of_stream = 0xFFFFFFFFu>
inline lUInt32 utfCodePointCount(const char_type* pstart, const char_type* pend) noexcept {
    lUInt32 count = 0;
    for(;;) {
        lChar32 ch = utfReadCodePoint<char_type, invalid_char, end_of_stream>(pstart, pend);
        if (ch == end_of_stream) {
            return count;
        }
        count++;
    }
    return count;
}

/// strcmp for two strings of different utf encoding, both args are non-empty, zero-terminated
template<typename char_type1, typename char_type2, typename size_type, lChar32 invalid_char = 0xFFFFFFFEu, lChar32 end_of_stream = 0xFFFFFFFFu>
inline int str_cmp_utf_nonempty(const char_type1 * s1, size_type len1, const char_type2 * s2, size_type len2) {
    // we are sure that sz1>0 && sz2>0
    const char_type1 * s1_end = s1 + len1;
    const char_type2 * s2_end = s2 + len2;
    for (;;) {
        lChar32 ch1 = utfReadCodePoint<char_type1, invalid_char, end_of_stream>(s1, s1_end);
        lChar32 ch2 = utfReadCodePoint<char_type2, invalid_char, end_of_stream>(s2, s2_end);
        if (ch1 == end_of_stream) {
            return (ch2 == end_of_stream) ? 0 : -1;
        }
        if (ch2 == end_of_stream) {
            return 1;
        }
        if (ch1 < ch2)
            return -1;
        if (ch1 > ch2)
            return 1;
    }
}



template <typename char_type, lChar32 invalid_char = '?'>
struct CodepointReadIterator {
    struct EndOfStream {};
    CodepointReadIterator(const char_type * start, const char_type * end) noexcept : pos(start), pend(end) { }
    CodepointReadIterator(const CodepointReadIterator& v) = default;
    ~CodepointReadIterator() = default;

    lChar32 operator*() const noexcept {
        const char_type * tmp = pos;
        return utfReadCodePoint<char_type, invalid_char>(tmp, pend);
    }
    CodepointReadIterator& operator++() {
        utfReadCodePoint<char_type, invalid_char>(pos, pend);
        return *this;
    }
    bool operator != (EndOfStream) const {
        return pos < pend;
    }
  private:
    const char_type * pos;
    const char_type * pend;
};

template <typename char_type, lChar32 invalid_char = '?'>
struct UtfDecodeRange {

    using iterator_t = CodepointReadIterator<char_type, invalid_char>;
    using sentinel_t = typename iterator_t::EndOfStream;

    UtfDecodeRange(const char_type * start, const char_type * end) noexcept : pstart(start), pend(end) { }
    UtfDecodeRange(const UtfDecodeRange& v) = default;
    ~UtfDecodeRange() noexcept = default;

    iterator_t begin() const { return iterator_t(pstart, pend); }
    sentinel_t end()   const { return sentinel_t(); } // Returns a completely different type!

  private:
    const char_type * pstart;
    const char_type * pend;
};


extern lChar32 fake_null_buffer_32;


/// writable copy of string (guaranteed to be empty or have reference counter == 1
/// can only be passed by reference created from string
/// allows modification without reference counting checks
template <typename char_type, typename refcounter_type = std::atomic_int>
class string_ro {
    // friend class string_wr<char_type, refcounter_type>;
    // friend class string<char_type, refcounter_type>;
    template <typename, typename> friend class string_ro;
    template <typename, typename> friend class string;
    template <typename, typename> friend class string_wr;
  public:
    // typedefs for STL compatibility
    typedef char_type             value_type;      ///< character type
    typedef lUInt32               size_type;       ///< size type
    typedef lInt32                difference_type; ///< difference type
    typedef value_type *          pointer;         ///< pointer to char type
    typedef value_type &          reference;       ///< reference to char type
    typedef const value_type *    const_pointer;   ///< pointer to const char type
    typedef const value_type &    const_reference; ///< reference to const char type
    //typedef string<char_type, refcounter_type> string_type; ///< normal shared string reference type

    // constant for not found / unspecified position
    static const size_type npos = -1;
    static lChar32 constexpr invalid_unicode = 0xfffffffe;
    static lChar32 constexpr end_of_stream = 0xffffffff;

    typedef UtfDecodeRange<char_type, invalid_unicode> utf_decode_range_t;
    typedef lstring_chunk_t<char_type, size_type, refcounter_type> chunk_t;

  protected:

    static const size_type STRING_HASH_MULT = 31;

    /// disabled: always created as a synonim of the string with the same template parameters
    string_ro() = default;
    /// don't free pchunk - it's owned by string
    ~string_ro() = default;
  public:

    // ============================================================================
    // size and capacity
    // ============================================================================

    /// returns true if string is empty
    bool empty() const noexcept { return pchunk == nullptr || pchunk->len==0; }
    /// returns character count
    size_type   length() const noexcept { return pchunk == nullptr ? 0 : pchunk->len; }
    /// returns character count (same as length)
    size_type   size() const noexcept { return pchunk == nullptr ? 0 : pchunk->len; }
    /// returns maximum number of chars that can fit into buffer (there is always additional one char space for trailing 0 which is not counted)
    size_type   capacity() const noexcept { return pchunk==nullptr ? 0 : pchunk->size; }

    // ============================================================================
    // item index methods
    // ============================================================================

    /// index value. No index nor empty string checks. Will crash on empty string.
    char_type operator [] (size_type index) const noexcept {
        return pchunk->buf[index];
    }

    /// index ref. No index nor refcount checks. Will crash on empty string. Only use on non-empy string which owns data (refcount==1).
    char_type& operator [] (size_type index) noexcept {
        return pchunk->buf[index];
    }

    /// index value with bounds checking. No index nor empty string checks. Will crash on empty string.
    /// Does not throw exception on index out of bounds, returns 0 instead.
    char_type at(size_type index) const noexcept {
        if (pchunk == nullptr || index >= pchunk->len) {
            return 0;
        }
        return pchunk->buf[index];
    }

    /// index ref. No index nor refcount checks. Will crash on empty string. Only use on non-empy string which owns data (refcount==1).
    /// Does not throw exception on index out of bounds, returns reference pointing to static 0 value instead.
    char_type& at(size_type index) noexcept {
        if (pchunk == nullptr || index >= pchunk->len) {
            return *(static_cast<char_type*>(&fake_null_buffer_32));
        }
        return pchunk->buf[index];
    }

    /// return C-style null-terminated string pointer
    const char_type * c_str() const noexcept {
        if (pchunk == nullptr) {
            return reinterpret_cast<const char_type *>(&fake_null_buffer_32);
        } else {
            // enforce null-termination
            const_cast<chunk_t*>(pchunk)->buf[pchunk->len] = 0;
            return pchunk->buf;
        }
    }

    /// returns last character from string, or 0 for empty string
    char_type lastChar() const noexcept {
        if (pchunk && pchunk->len) {
            return pchunk->buf[pchunk->len-1];
        } else {
            return 0;
        }
    }

    /// returns first character from string, or 0 for empty string
    char_type firstChar() const noexcept {
        if (pchunk && pchunk->len) {
            return pchunk->buf[0];
        } else {
            return 0;
        }
    }

    /// forward iterator - support for range by char_type
    const char_type * begin() const noexcept {
        if (pchunk) {
            return pchunk->buf;
        } else {
            return nullptr;
        }
    }

    /// forward iterator - support for range by char_type
    const char_type * end() const noexcept {
        if (pchunk) {
            return pchunk->buf + pchunk->len;
        } else {
            return nullptr;
        }
    }

    /// returns range object to read content as unicode codepoints -- use to iterate by codepoints
    /// * for sizeof(char_type) == 1 the content is interpreted as utf8 sequence
    /// * for sizeof(char_type) == 2 the content is interpreted as utf16 sequence
    /// * for sizeof(char_type) == 4 the content is interpreted as utf32 sequence
    /// if decoded codepoint value is not in valid unicode range, invalid_unicode value is returned
    utf_decode_range_t unicodeRange() const noexcept {
        if (pchunk && pchunk->len) {
            return utf_decode_range_t(pchunk->buf, pchunk->buf + pchunk->len);
        } else {
            return utf_decode_range_t(nullptr, nullptr);
        }
    }

    /// returns subrange object to read content as unicode codepoints -- use to iterate by codepoints
    /// Decoding starts from item [start] and ends at item [start + len - 1]
    /// * for sizeof(char_type) == 1 the content is interpreted as utf8 sequence
    /// * for sizeof(char_type) == 2 the content is interpreted as utf16 sequence
    /// * for sizeof(char_type) == 4 the content is interpreted as utf32 sequence
    /// if decoded codepoint value is not in valid unicode range, invalid_unicode value is returned
    utf_decode_range_t unicodeRange(size_type start, size_type len) const noexcept {
        if (pchunk && start < pchunk->len && len) {
            if (start + len > pchunk->len) {
                len = pchunk->len - start;
            }
            return utf_decode_range_t(pchunk->buf + start, pchunk->buf + start + len);
        } else {
            return utf_decode_range_t(nullptr, nullptr);
        }
    }

    /// assuming this string as utf-encoded (utf8/utf16/utf32), returns number of unicode codepoints inside the string.
    size_type codePointCount() const noexcept {
        if (pchunk && pchunk->len) {
            return utfCodePointCount<char_type, invalid_unicode, end_of_stream>(pchunk->buf, pchunk->buf + pchunk->len);
        }
        return 0;
    }

    /// for substring assuming this string as utf-encoded (utf8/utf16/utf32), returns number of unicode codepoints inside the string.
    size_type codePointCount(size_type start, size_type len) const noexcept {
        if (pchunk && start < pchunk->len && len) {
            if (start + len > pchunk->len) {
                len = pchunk->len - start;
            }
            return utfCodePointCount<char_type, invalid_unicode, end_of_stream>(pchunk->buf + start, pchunk->buf + start + len);
        }
        return 0;
    }


    /// calculate hash code for string (empty string has hash==0)
    size_type getHash() const noexcept {
        lUInt32 res = 0;
        if (pchunk) {
            for (size_type i=0; i < pchunk->len; i++)
                res = res * STRING_HASH_MULT + pchunk->buf[i];
        }
        return res;
    }

    // ============================================================================
    // compare methods
    // ============================================================================

    /// compare this string with another string, returns -1 if this < s, 1 if this > s, 0 if equal
    int compare(const string_ro& s) const noexcept {
        if (pchunk == s.pchunk) {
            // same string
            return 0;
        }
        size_type sz1 = length();
        size_type sz2 = s.length();
        if (sz1 == 0) {
            return sz2 > 0 ? -1 : 0;
        } else if (sz2 == 0) {
            return 1;
        }
        return str_cmp_nonempty<char_type, size_type>(pchunk->buf, sz1, s.pchunk->buf, sz2);
    }

    /// compare this string with another string (optionally of different type), returns -1 if this < s, 1 if this > s, 0 if equal
    /// if string s has the same type as current string, use simple compare()
    /// if string s has different type, both this string and `s` are read as utf unicode characters flow and compared by unicode codepoint values
    template<typename utf_type>
    int compareUtf(const string_ro<utf_type, refcounter_type>& s) const noexcept {
        if constexpr (sizeof(char_type) == sizeof(utf_type)) {
            // for strings of the same character type, use simple compare()
            return compare(s);
        } else {
            if (static_cast<const void *>(pchunk) == static_cast<const void *>(s.pchunk)) {
                // same string
                return 0;
            }
            // for strings of different character type (e.g. utf8<->utf32) compare unicode codepoints
            size_type sz1 = length();
            size_type sz2 = s.length();
            if (sz1 == 0) {
                return sz2 > 0 ? -1 : 0;
            } else if (sz2 == 0) {
                return 1;
            }
            return str_cmp_utf_nonempty<char_type, utf_type, size_type>(pchunk->buf, sz1, s.pchunk->buf, sz2);
        }
    }

    /// compare substring (pos..pos+n) of this string with another string, returns -1 if this < s, 1 if this > s, 0 if equal
    int compare(size_type pos, size_type n, const string_ro& s) const noexcept {
        if (!pchunk || pos >= pchunk->len) {
            // this string fragment is empty
            return s.empty() ? 0 : -1;
        }
        // clamp this fragment size
        if (pos + n > pchunk->len) {
            n = pchunk->len - pos;
        }
        // n > 0
        size_type sz2 = s.length();
        if (sz2 == 0) {
            return 1;
        }
        return str_cmp_nonempty<char_type, size_type>(pchunk->buf + pos, n, s.pchunk->buf, sz2);
    }

    /// compare substring (pos..pos+n) of this string with substring of another string (pos2..pos2+n2), returns -1 if this < s, 1 if this > s, 0 if equal
    int compare(size_type pos, size_type n, const string_ro& s, size_type pos2, size_type n2) const noexcept {
        // check if another string fragment is empty
        bool s_empty = (!s.pchunk || pos2 >= s.pchunk->len);
        if (!pchunk || pos >= pchunk->len) {
            // this string fragment is empty
            return s_empty ? 0 : -1;
        }
        if (s_empty) {
            // this string non-empty, another string is empty
            return 1;
        }
        // both string fragments are non-empty
        // clamp this fragment size
        if (pos + n > pchunk->len) {
            n = pchunk->len - pos;
        }
        // clamp other fragment size
        if (pos2 + n2 > s.pchunk->len) {
            n2 = s.pchunk->len - pos2;
        }
        // n > 0, n2 > 0
        return str_cmp_nonempty<char_type, size_type>(pchunk->buf + pos, n, s.pchunk->buf + pos2, n2);
    }

    /// compare substring (pos..pos+n) of this string with substring of another string s of len n2, returns -1 if this < s, 1 if this > s, 0 if equal
    int compare(size_type pos, size_type n, const char_type * s, size_type n2) const noexcept {
        // check if another string fragment is empty
        bool s_empty = !s || !n2;
        if (!pchunk || pos >= pchunk->len) {
            // this string fragment is empty
            return s_empty ? 0 : -1;
        }
        if (s_empty) {
            // this string non-empty, another string is empty
            return 1;
        }
        // both string fragments are non-empty
        // clamp this fragment size
        if (pos + n > pchunk->len) {
            n = pchunk->len - pos;
        }
        // n > 0, n2 > 0
        return str_cmp_nonempty<char_type, size_type>(pchunk->buf + pos, n, s, n2);
    }

    /// compare substring (pos..pos+n) of this string with another string s, returns -1 if this < s, 1 if this > s, 0 if equal
    int compare(size_type pos, size_type n, const char_type * s) const noexcept {
        size_type n2 = (!s) ? 0 : str_len<char_type, size_type>(s);
        return compare(pos, n, s, n2);
    }

    /// compare this string with string literal, returns -1 if this < s, 1 if this > s, 0 if equal
    int compare(const char_type* s) const noexcept {
        size_t sz1 = length();
        if (sz1 == 0) {
            return (s != nullptr && *s != 0) ? -1 : 0;
        } else if (s == nullptr || *s == 0) {
            return 1;
        }
        return str_cmp_nonempty<char_type, size_type>(pchunk->buf, sz1, s);
    }

    /// returns true when strings are equal
    bool operator == (const string_ro& s) const noexcept {
        return compare(s) == 0;
    }

    /// returns true when strings are not equal
    bool operator != (const string_ro& s) const noexcept {
        return compare(s) != 0;
    }

    /// returns true when this string < s
    bool operator < (const string_ro& s) const noexcept {
        return compare(s) < 0;
    }

    /// returns true when this string >= s
    bool operator <= (const string_ro& s) const noexcept {
        return compare(s) <= 0;
    }

    /// returns true when this string > s
    bool operator > (const string_ro& s) const noexcept {
        return compare(s) > 0;
    }

    /// returns true when this string <= s
    bool operator >= (const string_ro& s) const noexcept {
        return compare(s) >= 0;
    }

    /// returns true when strings are equal
    bool operator == (const char_type * s) const noexcept {
        return compare(s) == 0;
    }

    /// returns true when strings are not equal
    bool operator != (const char_type * s) const noexcept {
        return compare(s) != 0;
    }

    /// returns true when this string < s
    bool operator < (const char_type * s) const noexcept {
        return compare(s) < 0;
    }

    /// returns true when this string <= s
    bool operator <= (const char_type * s) const noexcept {
        return compare(s) <= 0;
    }

    /// returns true when this string > s
    bool operator > (const char_type * s) const noexcept {
        return compare(s) > 0;
    }

    /// returns true when this string >= s
    bool operator >= (const char_type * s) const noexcept {
        return compare(s) >= 0;
    }


    // ============================================================================
    // search methods
    // ============================================================================

    /// return first position of char c inside string starting from positon pos, or npos if no char is found.
    size_type pos(char_type c, size_type start = 0) const noexcept {
        if (pchunk) {
            const char_type * buf = pchunk->buf;
            size_type len = pchunk->len;
            for (size_type i = start; i < len; i++) {
                if (buf[i] == c) {
                    return i;
                }
            }
        }
        return npos;
    }

    /// return first position of string s starting from positon pos, or npos if no char is found.
    size_type pos(const string_ro& s, size_type start = 0) const noexcept {
        if (pchunk && s.pchunk) {
            size_type len = pchunk->len;
            if (start >= len) {
                return npos;
            }
            len -= start;
            size_type slen = s.pchunk->len;
            if (slen > len) {
                return npos;
            }
            const char_type * sbuf = s.pchunk->buf;
            const char_type * buf = pchunk->buf + start;
            size_type maxstart = len - slen;
            for (size_type i = 0; i <= maxstart; i++) {
                size_type found = i + start;
                for (size_type j = 0; j < slen; j++) {
                    if (buf[i + j] != sbuf[j]) {
                        found = npos;
                        break;
                    }
                }
                if (found != npos) {
                    return found;
                }
            }
        }
        return npos;
    }

    /// return first position of null-term string starting from positon pos, or npos if no char is found.
    size_type pos(const char_type* s, size_type start = 0) const noexcept {
        if (pchunk && s && s[0]) {
            size_type len = pchunk->len;
            if (start >= len) {
                return npos;
            }
            len -= start;
            size_type slen = str_len<char_type,size_type>(s);
            if (slen > len) {
                return npos;
            }
            const char_type * buf = pchunk->buf + start;
            size_type maxstart = len - slen;
            for (size_type i = 0; i <= maxstart; i++) {
                size_type found = i + start;
                for (size_type j = 0; j < slen; j++) {
                    if (buf[i + j] != s[j]) {
                        found = npos;
                        break;
                    }
                }
                if (found != npos) {
                    return found;
                }
            }
        }
        return npos;
    }

    /// return last position of null-term string s, or npos if no char is found.
    size_type rpos(const char_type* s) const noexcept {
        if (pchunk && s && s[0]) {
            size_type len = pchunk->len;
            size_type slen = str_len<char_type,size_type>(s);
            if (slen > len) {
                return npos;
            }
            const char_type * buf = pchunk->buf;
            size_type maxstart = len - slen;
            for (size_type i = maxstart; ; i--) {
                size_type found = i;
                for (size_type j = 0; j < slen; j++) {
                    if (buf[i + j] != s[j]) {
                        found = npos;
                        break;
                    }
                }
                if (found != npos) {
                    return found;
                }
                if (i == 0) {
                    break;
                }
            }
        }
        return npos;
    }

    /// returns true if this string starts with s[0..count-1], pass count=npos to calculate string size internally
    bool startsWith(const char_type* s, size_type count = npos) const noexcept {
        if (pchunk && s && s[0]) {
            size_type len = pchunk->len;
            size_type slen = (count == npos) ? str_len<char_type,size_type>(s) : count;
            if (slen > len) {
                return false;
            }
            const char_type * buf = pchunk->buf;
            for (size_type j = 0; j < slen; j++) {
                if (buf[j] != s[j]) {
                    return false;
                }
            }
            return true;
        }
        return false;
    }

    // ============================================================================
    // startsWith / endsWith
    // ============================================================================

    /// returns true if this string ends with s[0..count-1], pass count=npos to calculate string size internally
    bool endsWith(const char_type* s, size_type count = npos) const noexcept {
        if (pchunk && s && s[0]) {
            size_type len = pchunk->len;
            size_type slen = (count == npos) ? str_len<char_type,size_type>(s) : count;
            if (slen > len) {
                return false;
            }
            const char_type * buf = pchunk->buf + (len - slen);
            for (size_type j = 0; j < slen; j++) {
                if (buf[j] != s[j]) {
                    return false;
                }
            }
            return true;
        }
        return false;
    }

    /// returns true if this string starts with s
    bool startsWith(const string_ro& s) const noexcept {
        if (pchunk && s.pchunk) {
            size_type len = pchunk->len;
            size_type slen = s.pchunk->len;
            if (slen > len) {
                return false;
            }
            const char_type * buf = pchunk->buf;
            const char_type * sbuf = s.pchunk->buf;
            for (size_type j = 0; j < slen; j++) {
                if (buf[j] != sbuf[j]) {
                    return false;
                }
            }
            return true;
        }
        return false;
    }

    /// returns true if this string starts with s
    bool endsWith(const string_ro& s) const noexcept {
        if (pchunk && s.pchunk) {
            size_type len = pchunk->len;
            size_type slen = s.pchunk->len;
            if (slen > len) {
                return false;
            }
            const char_type * buf = pchunk->buf + (len - slen);
            const char_type * sbuf = s.pchunk->buf;
            for (size_type j = 0; j < slen; j++) {
                if (buf[j] != sbuf[j]) {
                    return false;
                }
            }
            return true;
        }
        return false;
    }

    /// convert to integer
    int atoi() const noexcept {
        int sgn = 1;
        int n = 0;
        const char_type * s = c_str();
        // skip whitespace
        while (*s == ' ' || *s == '\t') {
            s++;
        }
        if (*s == '-') {
            sgn = -1;
            s++;
        } else if (*s == '+') {
            s++;
        }
        while (*s>='0' && *s<='9') {
            n = n * 10 + ( (*s)-'0' );
            s++;
        }
        return (sgn>0)?n:-n;
    }

    /// convert to integer
    bool atoi(int& n) const noexcept {
        n = 0;
        int sgn = 1;
        const char_type * s = c_str();
        // allow leading whitespace -- not an error
        while (*s == ' ' || *s == '\t')
            s++;
        // support hex with prefix 0x
        if ( s[0]=='0' && s[1]=='x') {
            s+=2;
            for (;*s;) {
                int d = hexDigit(*s++);
                if ( d>=0 )
                    n = (n<<4) | d;
            }
            return true;
        }
        if (*s == '-') {
            sgn = -1;
            s++;
        }
        else if (*s == '+') {
            s++;
        }
        if ( !(*s>='0' && *s<='9') )
            return false;
        while (*s>='0' && *s<='9') {
            if (n > 0x7fffffff/10) {
                return false;
            }
            n = n * 10 + ( (*s++)-'0' );
        }
        if ( sgn<0 )
            n = -n;
        return *s=='\0' || *s==' ' || *s=='\t';
    }

    /// convert string to 64-bit integer, supports +/- for decimal, and 0x prefix for hex, skips leading whitespace
    bool atoi(lInt64& n) const noexcept {
        n = 0;
        int sgn = 1;
        const char_type * s = c_str();
        // allow leading whitespace -- not an error
        while (*s == ' ' || *s == '\t')
            s++;
        // support hex with prefix 0x
        if ( s[0]=='0' && s[1]=='x') {
            s+=2;
            for (;*s;) {
                int d = hexDigit(*s++);
                if ( d>=0 )
                    n = (n<<4) | d;
            }
            return true;
        }
        if (*s == '-') {
            sgn = -1;
            s++;
        }
        else if (*s == '+') {
            s++;
        }
        if ( !(*s>='0' && *s<='9') )
            return false;
        while (*s>='0' && *s<='9') {
            if (n > 0x7fffffffffffffffll/10) {
                return false;
            }
            n = n * 10 + ( (*s++)-'0' );
        }
        if ( sgn<0 )
            n = -n;
        return *s=='\0' || *s==' ' || *s=='\t';
    }

    /// convert to 64 bit integer
    lInt64 atoi64() const noexcept {
        int sgn = 1;
        lInt64 n = 0;
        const lChar8 * s = c_str();
        while (*s == ' ' || *s == '\t')
            s++;
        if (*s == '-')
        {
            sgn = -1;
            s++;
        }
        else if (*s == '+')
        {
            s++;
        }
        while (*s>='0' && *s<='9')
        {
            n = n * 10 + ( (*s)-'0' );
            s++;
        }
        return (sgn>0) ? n : -n;
    }

  protected:
    /// member variables must follow in the same order as in string for reinterpret cast
    chunk_t * pchunk {nullptr};
};

/// writable copy of string (guaranteed to be empty or have reference counter == 1
/// can only be passed by reference created from string
/// allows modification without reference counting checks
template <typename char_type, typename refcounter_type = std::atomic_int>
class string_wr : public string_ro<char_type, refcounter_type> {
public:
    // typedefs for STL compatibility
    typedef char_type             value_type;      ///< character type
    typedef lUInt32               size_type;       ///< size type
    typedef lInt32                difference_type; ///< difference type
    typedef value_type *          pointer;         ///< pointer to char type
    typedef value_type &          reference;       ///< reference to char type
    typedef const value_type *    const_pointer;   ///< pointer to const char type
    typedef const value_type &    const_reference; ///< reference to const char type
    typedef string_ro<char_type, refcounter_type> ro_string_type; ///< readonly string type
    typedef string<char_type, refcounter_type> string_type; ///< normal shared string type

    // constant for not found / unspecified position
    static const size_type npos = -1;

private:
    using typename string_ro<char_type, refcounter_type>::chunk_t;
    using string_ro<char_type, refcounter_type>::pchunk;
public:
    using string_ro<char_type, refcounter_type>::empty;
    using string_ro<char_type, refcounter_type>::length;
    using string_ro<char_type, refcounter_type>::size;
    using string_ro<char_type, refcounter_type>::capacity;

    using string_ro<char_type, refcounter_type>::operator [];
    using string_ro<char_type, refcounter_type>::at;
    using string_ro<char_type, refcounter_type>::c_str;

    /// disabled: always created as a synonim of the string with the same template parameters
    string_wr() {}
    /// don't free pchunk - it's owned by string
    ~string_wr() {}
public:

    /// return C-style null-terminated string pointer
    const char_type * data() const noexcept {
        return c_str();
    }

    /// return modifable C-style null-terminated string pointer; for empty string returns pointer to fake empty z-string buffer
    char_type * data() noexcept {
        if (pchunk == nullptr) {
            return reinterpret_cast<char_type *>(&fake_null_buffer_32);
        } else {
            // enforce null-termination
            const_cast<chunk_t*>(pchunk)->buf[pchunk->len] = 0;
            return pchunk->buf;
        }
    }

    /// sets value to null string, freeing buffer
    string_wr& clear() noexcept {
        if (pchunk != nullptr) {
            // use free instead of release because we own this string and ref count should be 1 anyway
            chunk_t::free(pchunk);
            pchunk = nullptr;
        }
        return *this;
    }

    /// resets length to zero, preparing to modification with reserved size
    string_wr&  reset(size_type size) noexcept {
        if (pchunk == nullptr) {
            // buffer is allocated and has capacity at least size
            pchunk = chunk_t::alloc(size);
        } else {
            // this string is non-empty
            if (pchunk->size >= size) {
                // this is already our own string with enough capacity -- just reset length
                pchunk->len = 0;
            } else {
                // this string has not enough capacity for size, clear and alloc new
                chunk_t::free(pchunk);
                pchunk = chunk_t::alloc(size);
            }
        }
        return *this;
    }

    /// ensure that buffer has at least size capacity
    string_wr& reserve(size_type size) noexcept {
        if (size == 0) {
            // reserve(0) is a non-binding request, just treat this empty string as owned
            return *this;
        }
        if (pchunk != nullptr) {
            // already own buffer
            if (pchunk->size < size) {
                // only if size is not enough, create a bigger buffer copy
                chunk_t * tmp = pchunk->duplicate(size);
                chunk_t::free(pchunk);
                pchunk = tmp;
            }
        } else {
            // create new allocation of requested size
            pchunk = chunk_t::alloc(size);
            pchunk->len = 0;
            pchunk->buf[0] = 0;
        }
        return *this;
    }

    // Trim methods

    /// remove leading and trailing spaces and tabs
    string_wr& trim() noexcept {
        // do nothing for empty line
        if (!pchunk || !pchunk->len) {
            return *this;
        }
        // we own buffer
        size_type firstns = 0;
        size_type len = pchunk->len;
        char_type * buf = pchunk->buf;
        for (;
             firstns < len &&
             (buf[firstns] == ' ' ||
              buf[firstns] == '\t');
             ++firstns)
            ;
        if (firstns >= len) {
            // only whitespace chars in the string
            pchunk->len = 0;
            pchunk->buf[0] = 0;
            return *this;
        }
        size_type lastns = len - 1;
        for (;
             //lastns>0 &&  // this check is not needed - we know that string contains non-empty-space char(s)
             (buf[lastns]==' ' || buf[lastns]=='\t');
             --lastns)
            ;
        size_type newlen = (size_type)(lastns + 1 - firstns);
        if (newlen == len) {
            // nothing to trim
            return *this;
        }
        if (firstns) {
            std::memmove( buf, buf + firstns, newlen*sizeof(char_type) );
        }
        buf[newlen] = 0;
        pchunk->len = newlen;
        return *this;
    }


    /// compact buffer if possible -- free unused buffer space; returned reference is a normal string
    string_type& pack() noexcept {
        if (pchunk) {
            size_type new_size = chunk_t::alignSize(pchunk->len);
            if (pchunk->size > new_size) {
                chunk_t * tmp = chunk_t::alloc(pchunk->buf, pchunk->len, pchunk->len);
                chunk_t::free(pchunk);
                pchunk = tmp;
            }
        }
        return *reinterpret_cast<string_type*>(this);
    }

    /// move assignment - shared string
    ///
    string_wr& assign(ro_string_type&& s) noexcept {
        LS_COUNT_MOVE_ASSIGN

        if (&s == this || pchunk == s.pchunk) {
            // safe self assignment: do nothing
            return *this;
        }

        if (s.empty()) {
            // assign empty string
            if (pchunk) {
                // reset buffer length to 0, don't free the buffer
                pchunk->len = 0;
            }
            return *this;
        } else if (s.pchunk->isShared()) {
            // passed string is shared
            if (!pchunk) {
                // this buffer is null, need to copy
                pchunk = s.pchunk->duplicate();
            } else {
                // we have buffer, try to figure out if only content may be copied
                // this call will copy content if dest buffer has enough capacity, or alloc a copy otherwise
                s.pchunk->duplicate_to_owned(pchunk);
            }
        } else {
            // s is not shared, may borrow its buffer instead of copy
            if (pchunk) {
                // free our buffer
                chunk_t::free(pchunk);
            }
            pchunk = s.pchunk;
            s.pchunk = nullptr;
        }
        return *this;
    }

    /// move assignment - owned string
    string_wr& assign(string_wr&& s) noexcept {
        LS_COUNT_MOVE_ASSIGN
        if (&s == this || pchunk == s.pchunk) {
            // safe self assignment: do nothing
            return *this;
        }
        // self-assignment prevention
        if (pchunk != nullptr) {
            chunk_t::free(pchunk);
        }
        pchunk = s.pchunk;
        s.pchunk = nullptr;
        return *this;
    }

    /// copy assignment
    string_wr& assign(const ro_string_type& s) noexcept {
        LS_COUNT_COPY_ASSIGN

        if (&s == this || pchunk == s.pchunk) {
            // safe self assignment: do nothing
            return *this;
        }

        if (s.empty()) {
            // empty string assignment: just reset len
            if (pchunk)
                pchunk->len = 0;
        } else {
            // try reuse this buffer if it has enough capacity, or create duplicate otherwise
            s.pchunk->duplicate_to_owned(pchunk);
        }
        return *this;
    }

    /// fragment assignment; correctly covers self-assignment; doesn't check bounds
    string_wr& assign(const ro_string_type&s, size_type offset, size_type count) noexcept {
        if (s.pchunk && offset < s.pchunk->len) {
            s.pchunk->duplicate_to_owned(pchunk, offset, count);
        } else {
            // assignment of empty string: don't clear the buffer -- typically string_wr is created to do multiple updates
            if (pchunk)
                pchunk->len = 0;
        }
        return *this;
    }

    /// assign from z-terminated string
    string_wr& operator = (const char_type * s) noexcept {
        return assign(s);
    }

    /// assign from z-terminated string
    string_wr& assign(const char_type * s) noexcept {
        if (s == nullptr || !*s) {
            // assignment of empty string: don't clear the buffer -- typically string_wr is created to do multiple updates
            if (pchunk)
                pchunk->len = 0;
        } else {
            size_type count = str_len<char_type, size_type>(s);
            if (pchunk != nullptr && pchunk->size >= count) {
                // reuse existing buffer
                std::memcpy(pchunk->buf, s, count * sizeof(char_type));
                pchunk->len = count;
            } else {
                // create new buffer
                chunk_t * tmp = chunk_t::alloc(s, count, count);
                if (pchunk) {
                    chunk_t::free(pchunk);
                }
                pchunk = tmp;
            }
        }
        return *this;
    }

    /// assign from char pointer and char count
    string_wr& assign(const char_type * s, size_type count) noexcept {
        if (s == nullptr || count == 0) {
            // assignment of empty string: don't clear the buffer -- typically string_wr is created to do multiple updates
            if (pchunk)
                pchunk->len = 0;
        } else {
            if (pchunk != nullptr && pchunk->size >= count) {
                // reuse existing buffer
                std::memcpy(pchunk->buf, s, count * sizeof(char_type));
                pchunk->len = count;
            } else {
                // create new buffer
                chunk_t * tmp = chunk_t::alloc(s, count, count);
                if (pchunk) {
                    chunk_t::free(pchunk);
                }
                pchunk = tmp;
            }
        }
        return *this;
    }

    /// Assign utf string from char pointer and char count, perform UTF conversion and invalid unicode codepoints skipping if necessary.
    /// Source string must be encoded as utf characters (utf8 for 1-byte utf_type, utf16 for 2-byte utf_type, utf32 for 4-byte utf_type).
    /// Converted data will be assigned on this string in format depending on char_type size (utf8/utf16/utf32).
    /// s is a start of source utf string
    /// count is number of source string elements to convert.
    /// Pass `npos` as source string element counter to calculate it for null-term string with str_len
    template<typename utf_type>
    string_wr& assignUtf(const utf_type * s, size_type count = npos) noexcept {
        if (s == nullptr) {
            count = 0;
        } else if (count == npos) {
            count = str_len<utf_type, size_type>(s);
        }
        if (count == 0) {
            // assignment of empty string: don't clear the buffer -- typically string_wr is created to do multiple updates
            if (pchunk)
                pchunk->len = 0;
        } else {
            size_type sz = utfConvDestBufferSize<utf_type, char_type>(s, s + count);
            lUInt32 errorCount = 0;
            if (pchunk != nullptr && pchunk->size >= sz) {
                // reuse existing buffer
                char_type * dst = pchunk->buf;
                pchunk->len = utfConvert<utf_type,char_type>(s, s+count, dst, dst + pchunk->size, errorCount);
            } else {
                // create new buffer
                chunk_t * tmp = chunk_t::alloc(sz);
                char_type * dst = tmp->buf;
                tmp->len = utfConvert<utf_type,char_type>(s, s+count, dst, dst + tmp->size, errorCount);
                if (pchunk) {
                    chunk_t::free(pchunk);
                }
                pchunk = tmp;
            }
        }
        return *this;
    }

    /// Append utf string from char pointer and char count, perform UTF conversion and invalid unicode codepoints skipping if necessary.
    /// Source string must be encoded as utf characters (utf8 for 1-byte utf_type, utf16 for 2-byte utf_type, utf32 for 4-byte utf_type).
    /// Converted data will be assigned on this string in format depending on char_type size (utf8/utf16/utf32).
    /// s is a start of source utf string
    /// count is number of source string elements to convert.
    /// Pass `npos` as source string element counter to calculate it for null-term string with str_len
    template<typename utf_type>
    string_wr& appendUtf(const utf_type * s, size_type count = npos) noexcept {
        if (empty()) {
            return assignUtf(s, count);
        }
        if (s == nullptr) {
            return *this;
        } else if (count == npos) {
            count = str_len<utf_type, size_type>(s);
        }
        if (count == 0) {
            return *this;
        }
        size_type sz = utfConvDestBufferSize<utf_type, char_type>(s, s + count);
        lUInt32 errorCount = 0;
        if (pchunk->size >= pchunk->len + sz) {
            // reuse existing buffer - it is big enough, and not shared
            char_type * dst = pchunk->buf + pchunk->len;
            pchunk->len += utfConvert<utf_type,char_type>(s, s+count, dst, pchunk->buf + pchunk->size, errorCount);
        } else {
            // create new buffer
            chunk_t * tmp = pchunk->duplicate(pchunk->len + sz);
            char_type * dst = tmp->buf + tmp->len;
            tmp->len += utfConvert<utf_type,char_type>(s, s+count, dst, tmp->buf + tmp->size, errorCount);
            chunk_t::free(pchunk);
            pchunk = tmp;
        }
        return *this;
    }

    /// convert all characters of string to uppercase
    string_wr& uppercase() noexcept {
        if (pchunk) {
            lStr_uppercase(pchunk->buf, pchunk->len);
        }
        return *this;
    }

    /// convert all characters of string to lowercase
    string_wr& lowercase() noexcept {
        if (pchunk) {
            lStr_lowercase(pchunk->buf, pchunk->len);
        }
        return *this;
    }

    /// appends decimal string representation of integer value
    string_wr& appendDecimal(lInt64 n) noexcept {
        char_type buf[24];
        int i=0;
        int negative = 0;
        if (n==0) {
            return append(1, '0');
        } else if (n<0)
        {
            negative = 1;
            n = -n;
        }
        for ( ; n; n/=10 )
        {
            buf[i++] = '0' + (n % 10);
        }
        reserve(length() + i + negative);
        if (negative)
            append(1, '-');
        for (int j=i-1; j>=0; j--)
            append(1, buf[j]);
        return *this;
    }

    /// appends hex string representation of integer value, no leading zeroes
    string_wr& appendHex(lUInt64 n) noexcept {
        if (n == 0) {
            return append(1, '0');
        }
        reserve(length() + 16);
        bool foundNz = false;
        for (int i=0; i<16; i++) {
            int digit = (n >> 60) & 0x0F;
            if (digit) {
                foundNz = true;
            }
            if (foundNz) {
                append(1, (static_cast<char_type>("0123456789abcdef"[digit])) & 0xFF);
            }
            n <<= 4;
        }
        return *this;
    }

    /// append fragment from null-terminated string; no validation of input string is performed
    string_wr& append(const char_type* s, size_type count) noexcept {
        if (count) {
            if (!pchunk) {
                pchunk = chunk_t::alloc(s, count, count);
            } else {
                size_type new_len = pchunk->len + count;
                if (pchunk->size < new_len) {
                    // need new buffer: either shared or not enough capacity
                    // if s points to our own buffer, save data before releasing
                    bool self_append = (s == pchunk->buf);
                    chunk_t * tmp = chunk_t::alloc(pchunk->buf, pchunk->len, new_len);
                    if (self_append) {
                        // s was our old buffer, now freed; copy from the new tmp which has the old data
                        std::memcpy(tmp->buf + tmp->len, tmp->buf, sizeof(char_type) * count);
                    } else {
                        std::memcpy(tmp->buf + tmp->len, s, sizeof(char_type) * count);
                    }
                    tmp->len = new_len;
                    tmp->buf[new_len] = 0;
                    chunk_t::free(pchunk);
                    pchunk = tmp;
                } else {
                    std::memmove(pchunk->buf + pchunk->len, s, sizeof(char_type) * count);
                    pchunk->len = new_len;
                    pchunk->buf[new_len] = 0;
                }
            }
        }
        return *this;
    }

    /// append null-terminated string
    string_wr& append(const char_type* s) noexcept {
        if (!s || !*s) {
            return *this;
        }
        size_type count = str_len<char_type, size_type>(s);
        if (count) {
            append(s, count);
        }
        return *this;
    }

    /// append another string
    string_wr& append(const string_wr& s) noexcept {
        if (!s.empty()) {
            return append(s.pchunk->buf, s.pchunk->len);
        }
        return *this;
    }

    /// append fragment of another string
    string_wr& append(const string_wr& s, size_type offset, size_type count) noexcept {
        if (!s.empty() && offset < s.pchunk->len) {
            if (offset + count > s.pchunk->len) {
                count = s.pchunk->len - offset;
            }
            return append(s.pchunk->buf + offset, count);
        }
        return *this;
    }

    /// append one or more characters
    string_wr& append(size_type count, char_type c) noexcept {
        if (pchunk) {
            size_type new_len = pchunk->len + count;
            if (pchunk->size < new_len) {
                chunk_t * tmp = chunk_t::alloc(pchunk->buf, pchunk->len, new_len);
                for (size_type i = 0; i < count; i++) {
                    tmp->buf[pchunk->len + i] = c;
                }
                tmp->len = new_len;
                tmp->buf[new_len] = 0;
                chunk_t::free(pchunk);
                pchunk = tmp;
            } else {
                // own buffer with enough capacity
                for (size_type i = 0; i < count; i++) {
                    pchunk->buf[pchunk->len + i] = c;
                }
                pchunk->len = new_len;
                pchunk->buf[new_len] = 0;
            }
        } else {
            // appending to empty string
            pchunk = chunk_t::alloc(count);
            for (size_type i = 0; i < count; i++) {
                pchunk->buf[i] = c;
            }
            pchunk->len = count;
            pchunk->buf[count] = 0;
        }
        return *this;
    }

    /// append single character
    string_wr& operator << (char_type ch) noexcept { return append(1, ch); }
    /// append C-string
    string_wr& operator << (const char_type * str) noexcept { return append(str); }
    /// append string
    string_wr& operator << (const string_wr & str) noexcept { return append(str); }
    /// append decimal number
    string_wr& operator << (const fmt::decimal v) noexcept { return appendDecimal(v.get()); }
    /// append hex number
    string_wr& operator << (const fmt::hex v) noexcept { return appendHex(v.get()); }

    /// append single character
    string_wr& operator += (char_type ch) noexcept { return append(1, ch); }
    /// append C-string
    string_wr& operator += (const char_type * str) noexcept { return append(str); }
    /// append string
    string_wr& operator += (const string_wr & str) noexcept { return append(str); }
    /// append decimal number
    string_wr& operator += (const fmt::decimal v) noexcept { return appendDecimal(v.get()); }
    /// append hex number
    string_wr& operator += (const fmt::hex v) noexcept { return appendHex(v.get()); }


    /// insert from char* and size
    string_wr& insert(size_type pos, const char_type* s, size_type count) noexcept {
        if (!count) {
            return *this;
        }
        if (!pchunk) {
            // insert into empty string
            pchunk = chunk_t::alloc(s, count, count);
        } else {
            if (pos > pchunk->len) {
                pos = pchunk->len;
            }
            size_type new_len = pchunk->len + count;
            size_type tail_len = pchunk->len - pos;
            if (new_len > pchunk->size) {
                // create copy
                chunk_t* tmp = chunk_t::alloc(new_len);
                if (pos > 0) {
                    // copy head
                    std::memcpy(tmp->buf, pchunk->buf, sizeof(char_type) * pos);
                }
                // copy inserted content
                std::memcpy(tmp->buf + pos, s, sizeof(char_type) * count);
                // copy tail if needed
                if (tail_len) {
                    // copy tail
                    std::memcpy(tmp->buf + pos + count, pchunk->buf + pos, sizeof(char_type) * tail_len);
                }
                tmp->len = new_len;
                chunk_t::free(pchunk);
                pchunk = tmp;
            } else {
                // insert in-place
                if (tail_len) {
                    // move tail by count chars
                    std::memmove(pchunk->buf + pos + count, pchunk->buf + pos, sizeof(char_type) * tail_len);
                }
                // copy inserted content
                std::memcpy(pchunk->buf + pos, s, sizeof(char_type) * count);
            }
            pchunk->len = new_len;
            pchunk->buf[new_len] = 0;
        }
        return *this;
    }

    /// insert from null-terminated cont char*
    string_wr& insert(size_type pos, const char_type* s) noexcept {
        if (!s || !*s) {
            // attempt inserting empty string
            return *this;
        }
        return insert(pos, s, str_len<char_type, size_type>(s));
    }

    /// insert from string (no self-insert protection)
    string_wr& insert(size_type pos, const ro_string_type& s) noexcept {
        if (s.empty()) {
            // attempt inserting empty string
            return *this;
        }
        return insert(pos, s.pchunk->buf, s.pchunk->len);
    }

    /// insert one or more characters
    string_wr& insert(size_type pos, size_type count, char_type c) noexcept {
        if (!count) {
            return *this;
        }
        if (!pchunk) {
            // insert into empty string
            pchunk = chunk_t::alloc(count);
            for (size_type i = 0; i < count; i++) {
                pchunk->buf[i] = c;
            }
            pchunk->len = count;
            pchunk->buf[count] = 0;
        } else {
            if (pos > pchunk->len) {
                pos = pchunk->len;
            }
            size_type new_len = pchunk->len + count;
            size_type tail_len = pchunk->len - pos;
            if (new_len > pchunk->size) {
                // create copy
                chunk_t* tmp = chunk_t::alloc(new_len);
                if (pos > 0) {
                    // copy head
                    std::memcpy(tmp->buf, pchunk->buf, sizeof(char_type) * pos);
                }
                // copy inserted content
                for (size_type i = 0; i < count; i++) {
                    tmp->buf[pos + i] = c;
                }
                // copy tail if needed
                if (tail_len) {
                    // copy tail
                    std::memcpy(tmp->buf + pos + count, pchunk->buf + pos, sizeof(char_type) * tail_len);
                }
                tmp->len = new_len;
                chunk_t::free(pchunk);
                pchunk = tmp;
            } else {
                // insert in-place
                if (tail_len) {
                    // move tail by count chars
                    std::memmove(pchunk->buf + pos + count, pchunk->buf + pos, sizeof(char_type) * tail_len);
                }
                // copy inserted content
                for (size_type i = 0; i < count; i++) {
                    pchunk->buf[pos + i] = c;
                }
            }
            pchunk->len = new_len;
            pchunk->buf[new_len] = 0;
        }
        return *this;
    }

    /// replace part of string with count chars from char*
    string_wr& replace(size_type pos, size_type n, const char_type * s, size_type count) noexcept {
        if (!count) {
            return *this;
        }
        if (empty()) {
            return assign(s, count);
        } else {
            // need to modify
            size_type old_len = pchunk->len;
            if (pos > old_len) {
                pos = old_len;
            }
            if (pos + n > old_len) {
                n = old_len - pos;
            }
            if (!n) {
                // delegate to insert
                return insert(pos, s, count);
            }
            if (pos == old_len) {
                // use append
                return append(s, count);
            }
            size_type new_len = old_len + count - n;
            size_type tail_len = old_len - (pos + n);
            if (new_len > pchunk->size) {
                // need to create copy -- no space or has other refs
                chunk_t * tmp = chunk_t::alloc(pchunk->buf, new_len, new_len);
                // move tail to the end of buffer
                std::memmove(tmp->buf + new_len - tail_len, tmp->buf + old_len - tail_len, sizeof(char_type) * tail_len);
                // fill gap with inserted string
                std::memcpy(tmp->buf + pos, s, sizeof(char_type) * count);
                // replace this string with new
                chunk_t::free(pchunk);
                pchunk = tmp;
            } else {
                // may edit in-place
                // move tail to the end of buffer
                std::memmove(pchunk->buf + new_len - tail_len, pchunk->buf + old_len - tail_len, sizeof(char_type) * tail_len);
                // fill gap with inserted string
                std::memcpy(pchunk->buf + pos, s, sizeof(char_type) * count);
            }
            // update length
            pchunk->len = new_len;
            pchunk->buf[new_len] = 0;
            return *this;
        }
    }

    /// replace part of string with count chars from char*
    string_wr& replace(size_type pos, size_type n, size_type count, char_type c) noexcept {
        if (!count) {
            // nothing to insert
            return *this;
        }
        if (empty()) {
            return append(count, c);
        } else {
            // need to modify
            size_type old_len = pchunk->len;
            if (pos > old_len) {
                pos = old_len;
            }
            if (pos + n > old_len) {
                n = old_len - pos;
            }
            if (!n) {
                // delegate to insert
                return insert(pos, count, c);
            }
            if (pos == old_len) {
                // use append
                return append(count, c);
            }
            size_type new_len = old_len + count - n;
            size_type tail_len = old_len - (pos + n);
            if (new_len > pchunk->size) {
                // need to create copy -- no space or has other refs
                chunk_t * tmp = chunk_t::alloc(pchunk->buf, new_len, new_len);
                // move tail to the end of buffer
                std::memmove(tmp->buf + new_len - tail_len, tmp->buf + old_len - tail_len, sizeof(char_type) * tail_len);
                // fill gap with inserted string
                for (size_type i = 0; i < count; i++) {
                    tmp->buf[pos + i] = c;
                }
                // replace this string with new
                chunk_t::free(pchunk);
                pchunk = tmp;
            } else {
                // may edit in-place
                // move tail to the end of buffer
                std::memmove(pchunk->buf + new_len - tail_len, pchunk->buf + old_len - tail_len, sizeof(char_type) * tail_len);
                // fill gap with inserted string
                for (size_type i = 0; i < count; i++) {
                    pchunk->buf[pos + i] = c;
                }
            }
            // update length
            pchunk->len = new_len;
            pchunk->buf[new_len] = 0;
            return *this;
        }
    }

    /// replace part of string with count chars from char*
    string_wr& replace(size_type pos, size_type n, const char_type * s) noexcept {
        if (!s || !*s) {
            // nothing to insert
            return *this;
        }
        size_type count = str_len<char_type, size_type>(s);
        return replace(pos, n, s, count);
    }

    /// replace part of string with count chars from char*
    string_wr& replace(size_type pos, size_type n, const ro_string_type& s) noexcept {
        if (s.empty()) {
            // nothing to insert
            return *this;
        }
        return replace(pos, n, s.pchunk->buf, s.pchunk->len);
    }

    /// replace part of string with count chars from char*
    string_wr& replace(size_type pos, size_type n, const ro_string_type& s, size_type offset, size_type count) noexcept {
        if (s.empty()) {
            // nothing to insert
            return *this;
        }
        size_type s_len = s.pchunk->len;
        if (offset >= s_len) {
            // source data indexes outside source string bounds
            return *this;
        }
        // ensure source range is inside string - truncate if needed
        if (offset + count > s_len) {
            count = s_len - offset;
        }
        return replace(pos, n, s.pchunk->buf + offset, count);
    }

    /// replace all occurences of character replaceWhat with character replaceTo
    string_wr& replace(char_type replaceWhat, char_type replaceTo) noexcept {
        if (empty()) {
            // nothing to replace
            return *this;
        }
        size_type len = pchunk->len;
        char_type * buf = pchunk->buf;
        for (size_type i = 0; i < len; i++) {
            if (buf[i] == replaceWhat) {
                // first match found: replace is needed
                // own buffer : replace in-place
                buf[i++] = replaceTo;
                while (i < len) {
                    if (buf[i] == replaceWhat) {
                        buf[i] = replaceTo;
                    }
                    i++;
                }
                break;
            }
        }
        return *this;
    }

    /// erases fragment from string; if requested fragment exceeds string bounds, it's size is truncated
    string_wr& erase(size_type offset, size_type count) noexcept {
        if (pchunk && offset < pchunk->len && count) {
            if (offset + count > pchunk->len) {
                // truncate requested erased fragment size to not exceed bounds
                count = pchunk->len - offset;
            }
            size_type new_len = pchunk->len - count;
            size_type tail_start = offset + count;
            size_type tail_len = pchunk->len - tail_start;
            // erase inplace
            if (tail_len) {
                std::memmove(pchunk->buf + offset, pchunk->buf + tail_start, sizeof(char_type) * tail_len);
            }
            pchunk->len = new_len;
            pchunk->buf[pchunk->len] = 0;
        }
        return *this;
    }
};

template <typename char_type, typename refcounter_type = std::atomic_int>
class string : public string_ro<char_type, refcounter_type> {
    friend class string_wr<char_type, refcounter_type>;
public:
    // typedefs for STL compatibility
    typedef char_type             value_type;      ///< character type
    typedef lUInt32               size_type;       ///< size type
    typedef lInt32                difference_type; ///< difference type
    typedef value_type *          pointer;         ///< pointer to char type
    typedef value_type &          reference;       ///< reference to char type
    typedef const value_type *    const_pointer;   ///< pointer to const char type
    typedef const value_type &    const_reference; ///< reference to const char type

    // constant for not found / unspecified position
    static const size_type npos = -1;

    // empty string constant
    //static const string empty_str;
    //static string constexpr empty_str = nullptr;
    inline static const string empty_str{};

    // COW types
    typedef string_wr<char_type, refcounter_type> writable_string; ///< writable (owned) string
    typedef string_wr<char_type, refcounter_type>& writable_ref;   ///< writable (owned) string reference
    typedef string_ro<char_type, refcounter_type> ro_string_type; ///< readonly string type

private:

  using typename string_ro<char_type, refcounter_type>::chunk_t;
  using string_ro<char_type, refcounter_type>::pchunk;

public:
  using string_ro<char_type, refcounter_type>::empty;
  using string_ro<char_type, refcounter_type>::length;
  using string_ro<char_type, refcounter_type>::size;
  using string_ro<char_type, refcounter_type>::capacity;

  using string_ro<char_type, refcounter_type>::operator [];
  using string_ro<char_type, refcounter_type>::at;
  using string_ro<char_type, refcounter_type>::c_str;
  //using string_ro<char_type, refcounter_type>::data;


public:
    string() noexcept = default;
    string(const char_type* s, size_type count, size_type reserved) noexcept {
        if (count == 0 || s == nullptr || s[0] == 0) {
            pchunk = nullptr;
        } else {
            pchunk = chunk_t::alloc(s, count, reserved);
        }
    }
    string(const char_type* s, size_type count) noexcept {
        if (count == 0 || s == nullptr || s[0] == 0) {
            pchunk = nullptr;
        } else {
            pchunk = chunk_t::alloc(s, count, count);
        }
    }
    explicit string(const char_type* s) noexcept {
        if (s != nullptr && *s) {
            pchunk = chunk_t::alloc(s, str_len<char_type, size_type>(s), 0);
        } else {
            pchunk = nullptr;
        }
    }
    /// copy constructor
    string(const string&s) noexcept {
        LS_COUNT_COPY_CONSTR
        pchunk = s.pchunk;
        intrusive_ptr_add_ref_checknull(pchunk);
    }
    /// constructor of empty buffer with reserved size
    string(size_type size) noexcept {
        pchunk = chunk_t::alloc(size);
    }
    /// move constructor
    string(string&&s ) noexcept {
        LS_COUNT_MOVE_CONSTR
        pchunk = s.pchunk;
        s.pchunk = nullptr;
    }
    /// fragment constructor
    string(const string&s, size_type offset, size_type count) noexcept {
        if (s.pchunk && offset < s.pchunk->len) {
            size_type avail = s.pchunk->len - offset;
            if (count > avail) count = avail;
            pchunk = chunk_t::alloc(s.pchunk->buf + offset, count, count);
        } else {
            pchunk = nullptr;
        }
    }
    /// destructor - frees reference
    ~string() noexcept {
        if (pchunk != nullptr) {
            intrusive_ptr_release(pchunk);
        }
    }

    /// move assignment
    string& operator = (string&& s) noexcept {
        LS_COUNT_MOVE_ASSIGN
        if (this != &s) {
            if (pchunk != nullptr) {
                intrusive_ptr_release(pchunk);
            }
            pchunk = s.pchunk;
            s.pchunk = nullptr;
        }
        return *this;
    }
    /// copy assignment
    string& operator = (const string& s) noexcept {
        return assign(s);
    }

    /// move assignment
    string& assign(string&& s) noexcept {
        LS_COUNT_MOVE_ASSIGN
        if (this != &s) {
            if (pchunk != nullptr) {
                intrusive_ptr_release(pchunk);
            }
            pchunk = s.pchunk;
            s.pchunk = nullptr;
        }
        return *this;
    }

    /// copy assignment
    string& assign(const string& s) noexcept {
        LS_COUNT_COPY_ASSIGN
        if (&s == this || pchunk == s.pchunk) {
            // safe self assignment: do nothing
            return *this;
        }
        if (pchunk != nullptr) {
            // ignore self-assignment
            intrusive_ptr_release(pchunk);
            pchunk = s.pchunk;
            if (pchunk) {
                // assigned non-empty string
                intrusive_ptr_add_ref(pchunk);
            }
        } else {
            // this string is null
            pchunk = s.pchunk;
            if (pchunk) {
                // assigned non-empty string
                intrusive_ptr_add_ref(pchunk);
            }
        }
        return *this;
    }

    /// fragment assignment; correctly covers self-assignment; doesn't check bounds
    string& assign(const ro_string_type&s, size_type offset, size_type count) noexcept {
        if (s.pchunk && offset < s.pchunk->len) {
            size_type avail = s.pchunk->len - offset;
            if (count > avail) count = avail;
            chunk_t* tmp = chunk_t::alloc(s.pchunk->buf + offset, count, count);
            if (pchunk) {
                intrusive_ptr_release(pchunk);
            }
            pchunk = tmp;
        } else {
            clear();
        }
        return *this;
    }

    /// assign from z-terminated string
    string& operator = (const char_type * s) noexcept {
        return assign(s);
    }

    /// assign from z-terminated string
    string& assign(const char_type * s) noexcept {
        if (s == nullptr || !*s) {
            clear();
        } else {
            size_type count = str_len<char_type, size_type>(s);
            if (pchunk != nullptr && pchunk->isOwn() && pchunk->size >= count && pchunk->size / 2 < count) {
                // reuse existing buffer
                std::memcpy(pchunk->buf, s, count * sizeof(char_type));
                pchunk->len = count;
            } else {
                // create new buffer
                chunk_t * tmp = chunk_t::alloc(s, count, count);
                if (pchunk) {
                    intrusive_ptr_release(pchunk);
                }
                pchunk = tmp;
            }
        }
    }

    /// assign from char pointer and char count
    string& assign(const char_type * s, size_type count) noexcept {
        if (s == nullptr || !*s || count == 0) {
            clear();
        } else {
            if (pchunk != nullptr && pchunk->isOwn() && pchunk->size >= count && pchunk->size / 2 < count) {
                // reuse existing buffer
                std::memcpy(pchunk->buf, s, count * sizeof(char_type));
                pchunk->len = count;
            } else {
                // create new buffer
                chunk_t * tmp = chunk_t::alloc(s, count, count);
                if (pchunk) {
                    intrusive_ptr_release(pchunk);
                }
                pchunk = tmp;
            }
        }
        return *this;
    }

    /// Assign utf string from char pointer and char count, perform UTF conversion and invalid unicode codepoints skipping if necessary.
    /// Source string must be encoded as utf characters (utf8 for 1-byte utf_type, utf16 for 2-byte utf_type, utf32 for 4-byte utf_type).
    /// Converted data will be assigned on this string in format depending on char_type size (utf8/utf16/utf32).
    /// s is a start of source utf string
    /// count is number of source string elements to convert.
    /// Pass `npos` as source string element counter to calculate it for null-term string with str_len
    template<typename utf_type>
    string& assignUtf(const utf_type * s, size_type count = npos) noexcept {
        if (s == nullptr) {
            count = 0;
        } else if (count == npos) {
            count = str_len<utf_type, size_type>(s);
        }
        if (count == 0) {
            clear();
        } else {
            size_type sz = utfConvDestBufferSize<utf_type, char_type>(s, s + count);
            lUInt32 errorCount = 0;
            if (pchunk != nullptr && pchunk->size >= sz && pchunk->isOwn()) {
                // reuse existing buffer - it is big enough, and not shared
                char_type * dst = pchunk->buf;
                pchunk->len = utfConvert<utf_type,char_type>(s, s+count, dst, dst + pchunk->size, errorCount);
            } else {
                // create new buffer
                chunk_t * tmp = chunk_t::alloc(sz);
                char_type * dst = tmp->buf;
                tmp->len = utfConvert<utf_type,char_type>(s, s+count, dst, dst + tmp->size, errorCount);
                if (pchunk) {
                    intrusive_ptr_release(pchunk);
                }
                pchunk = tmp;
            }
        }
        return *this;
    }

    /// Append utf string from char pointer and char count, perform UTF conversion and invalid unicode codepoints skipping if necessary.
    /// Source string must be encoded as utf characters (utf8 for 1-byte utf_type, utf16 for 2-byte utf_type, utf32 for 4-byte utf_type).
    /// Converted data will be assigned on this string in format depending on char_type size (utf8/utf16/utf32).
    /// s is a start of source utf string
    /// count is number of source string elements to convert.
    /// Pass `npos` as source string element counter to calculate it for null-term string with str_len
    template<typename utf_type>
    string& appendUtf(const utf_type * s, size_type count = npos) noexcept {
        if (empty()) {
            return assignUtf(s, count);
        }
        if (s == nullptr) {
            return *this;
        } else if (count == npos) {
            count = str_len<utf_type, size_type>(s);
        }
        if (count == 0) {
            return *this;
        }
        size_type sz = utfConvDestBufferSize<utf_type, char_type>(s, s + count);
        lUInt32 errorCount = 0;
        if (pchunk->size >= pchunk->len + sz && pchunk->isOwn()) {
            // reuse existing buffer - it is big enough, and not shared
            char_type * dst = pchunk->buf + pchunk->len;
            pchunk->len += utfConvert<utf_type,char_type>(s, s+count, dst, pchunk->buf + pchunk->size, errorCount);
        } else {
            // create new buffer
            chunk_t * tmp = pchunk->duplicate(pchunk->len + sz);
            char_type * dst = tmp->buf + tmp->len;
            tmp->len += utfConvert<utf_type,char_type>(s, s+count, dst, tmp->buf + tmp->size, errorCount);
            if (pchunk) {
                intrusive_ptr_release(pchunk);
            }
            pchunk = tmp;
        }
        return *this;
    }

    /// sets value to null string, freeing buffer
    string& clear() noexcept {
        if (pchunk != nullptr) {
            intrusive_ptr_release(pchunk);
            pchunk = nullptr;
        }
        return *this;
    }

    /// resets length to zero, preparing to modification with reserved size
    writable_string&  reset(size_type size) noexcept {
        if (pchunk == nullptr) {
            // buffer is allocated and has capacity at least size
            pchunk = chunk_t::alloc(size);
        } else {
            // this string is non-empty
            if (pchunk->isOwn() && pchunk->size >= size) {
                // this is already our own string with enough capacity -- just reset length
                pchunk->len = 0;
            } else {
                // this string has not enough capacity for size, clear and alloc new
                intrusive_ptr_release(pchunk);
                pchunk = chunk_t::alloc(size);
            }
        }
        return *reinterpret_cast<writable_string*>(this);
    }

    /// ensure that this string owns buffer and it has at least size capacity
    writable_string& reserve(size_type size) noexcept {
        if (size == 0) {
            // reserve(0) is a non-binding request, just treat this empty string as owned
            return *reinterpret_cast<writable_string*>(this);
        }
        if (pchunk != nullptr) {
            if (pchunk->isOwn()) {
                // already own buffer
                if (pchunk->size < size) {
                    // only if size is not enough, create a bigger buffer copy
                    chunk_t * tmp = chunk_t::alloc(size);
                    tmp->len = pchunk->len;
                    std::memcpy(tmp->buf, pchunk->buf, pchunk->len);
                    tmp->buf[tmp->len] = 0;
                    chunk_t::free(pchunk);
                    pchunk = tmp;
                }
            } else {
                // we don't own the string, need to create a copy
                if (size < pchunk->len) {
                    // when requested size is smaller than existing string len, ensure we will not loss chars
                    size = pchunk->len;
                }
                chunk_t * tmp = chunk_t::alloc(size);
                tmp->len = pchunk->len;
                std::memcpy(tmp->buf, pchunk->buf, pchunk->len);
                tmp->buf[tmp->len] = 0;
                intrusive_ptr_release(pchunk);
                pchunk = tmp;
            }
        } else {
            // create new allocation of requested size
            pchunk = chunk_t::alloc(size);
            pchunk->len = 0;
            pchunk->buf[0] = 0;
        }
        return *reinterpret_cast<writable_string*>(this);
    }

    /// get writable reference for own string - create a copy only if refcount > 1
    writable_string& writableRef() {
        if (pchunk && pchunk->isShared()) {
            // create a copy with refcount==1
            chunk_t * tmp = chunk_t::alloc(pchunk->buf, pchunk->len, pchunk->len);
            intrusive_ptr_release(pchunk);
            pchunk = tmp;
        }
        return *reinterpret_cast<writable_string*>(this);
    }

    /// get writable reference for own string - create a copy if refcount > 1
    /// if own copy has to be created, reserve size
    writable_string& writableRef(size_type size) {
        if (pchunk && pchunk->isShared()) {
            // create a copy with refcount==1
            chunk_t * tmp = chunk_t::alloc(pchunk->buf, pchunk->len, size);
            intrusive_ptr_release(pchunk);
            pchunk = tmp;
        }
        return *reinterpret_cast<writable_string*>(this);
    }

    /// resize string to specified length, filling new positions with e if expanded
    writable_string& resize(size_type size, char_type e = 0) noexcept {
        if (pchunk == nullptr) {
            // empty string: allocate new buffer
            pchunk = chunk_t::alloc(size);
            for (size_type i = 0; i < size; i++)
                pchunk->buf[i] = e;
            pchunk->len = size;
            pchunk->buf[size] = 0;
        } else {
            // non-empty string
            size_type oldlen = pchunk->len;
            if (pchunk->isOwn() && pchunk->size >= size) {
                // own buffer with enough capacity: just adjust length
                if (size > oldlen) {
                    for (size_type i = oldlen; i < size; i++)
                        pchunk->buf[i] = e;
                }
                pchunk->len = size;
                pchunk->buf[size] = 0;
            } else {
                // need new buffer
                chunk_t * tmp = chunk_t::alloc(pchunk->buf, oldlen < size ? oldlen : size, size);
                if (size > oldlen) {
                    for (size_type i = oldlen; i < size; i++)
                        tmp->buf[i] = e;
                }
                tmp->len = size;
                tmp->buf[size] = 0;
                intrusive_ptr_release(pchunk);
                pchunk = tmp;
            }
        }
        return *reinterpret_cast<writable_string*>(this);
    }

    /// convert all characters of string to uppercase
    string& uppercase() noexcept {
        if (pchunk) {
            writableRef().uppercase();
        }
        return *this;
    }

    /// convert all characters of string to lowercase
    string& lowercase() noexcept {
        if (pchunk) {
            writableRef().lowercase();
        }
        return *this;
    }

    /// remove leading and trailing spaces and tabs
    string& trim() noexcept {
        // do nothing for empty line
        if (!pchunk || !pchunk->len) {
            return *this;
        }
        size_type firstns = 0;
        size_type len = pchunk->len;
        char_type * buf = pchunk->buf;
        for (;
             firstns < len &&
             (buf[firstns] == ' ' ||
              buf[firstns] == '\t');
             ++firstns)
            ;
        if (firstns >= len) {
            // only whitespace chars in the string
            return clear();
        }
        size_type lastns = len - 1;
        for (;
               //lastns>0 &&  // this check is not needed - we know that string contains non-empty-space char(s)
             (buf[lastns]==' ' || buf[lastns]=='\t');
             --lastns)
            ;
        size_type newlen = (size_type)(lastns + 1 - firstns);
        if (newlen == len) {
            // nothing to trim
            return *this;
        }
        if (pchunk->isOwn()) {
            if (firstns) {
                std::memmove( buf, buf + firstns, newlen*sizeof(char_type) );
            }
            buf[newlen] = 0;
            pchunk->len = newlen;
        } else {
            chunk_t * tmp = chunk_t::alloc(buf + firstns, newlen, newlen);
            intrusive_ptr_release(pchunk);
            pchunk = tmp;
        }
        return *this;
    }

    /// trim non-alphanumeric characters from beginning and end of string
    string& trimNonAlpha() noexcept {
        // TODO: implement
        return *this;
    }

    /// erases fragment from string; if requested fragment exceeds string bounds, it's size is truncated
    string& erase(size_type offset, size_type count) noexcept {
        if (pchunk && offset < pchunk->len && count) {
            if (offset + count > pchunk->len) {
                // truncate requested erased fragment size to not exceed bounds
                count = pchunk->len - offset;
            }
            size_type new_len = pchunk->len - count;
            size_type tail_start = offset + count;
            size_type tail_len = pchunk->len - tail_start;
            if (pchunk->isOwn()) {
                // erase inplace
                if (tail_len) {
                    std::memmove(pchunk->buf + offset, pchunk->buf + tail_start, sizeof(char_type) * tail_len);
                }
            } else {
                // create a copy with fragment erased
                chunk_t * tmp = chunk_t::alloc(new_len);
                if (offset) {
                    // something left in beginning of string
                    std::memcpy(tmp->buf, pchunk->buf, sizeof(char_type) * offset);
                }
                if (tail_len) {
                    std::memcpy(tmp->buf + offset, pchunk->buf + tail_start, sizeof(char_type) * tail_len);
                }
                // free old ref and use copy
                intrusive_ptr_release(pchunk);
                pchunk = tmp;
            }
            pchunk->len = new_len;
            pchunk->buf[pchunk->len] = 0;
        }
        return *this;
    }

    /// appends decimal string representation of integer value
    string& appendDecimal(lInt64 n) noexcept {
        char_type buf[24];
        int i=0;
        int negative = 0;
        if (n==0) {
            return append(1, '0');
        } else if (n<0)
        {
            negative = 1;
            n = -n;
        }
        for ( ; n; n/=10 )
        {
            buf[i++] = '0' + (n % 10);
        }
        reserve(length() + i + negative);
        if (negative)
            append(1, '-');
        for (int j=i-1; j>=0; j--)
            append(1, buf[j]);
        return *this;
    }

    /// appends hex string representation of integer value, no leading zeroes
    string& appendHex(lUInt64 n) noexcept {
        if (n == 0) {
            return append(1, '0');
        }
        reserve(length() + 16);
        bool foundNz = false;
        for (int i=0; i<16; i++) {
            int digit = (n >> 60) & 0x0F;
            if (digit) {
                foundNz = true;
            }
            if (foundNz) {
                append(1, (static_cast<char_type>("0123456789abcdef"[digit])) & 0xFF);
            }
            n <<= 4;
        }
        return *this;
    }

    /// append fragment from null-terminated string; no validation of input string is performed
    string& append(const char_type* s, size_type count) noexcept {
        if (count) {
            if (!pchunk) {
                pchunk = chunk_t::alloc(s, count, count);
            } else {
                size_type new_len = pchunk->len + count;
                if (pchunk->isShared() || pchunk->size < new_len) {
                    // need new buffer: either shared or not enough capacity
                    // if s points to our own buffer, save data before releasing
                    bool self_append = (s == pchunk->buf);
                    chunk_t * tmp = chunk_t::alloc(pchunk->buf, pchunk->len, new_len);
                    if (self_append) {
                        // s was our old buffer, now freed; copy from the new tmp which has the old data
                        std::memcpy(tmp->buf + tmp->len, tmp->buf, sizeof(char_type) * count);
                    } else {
                        std::memcpy(tmp->buf + tmp->len, s, sizeof(char_type) * count);
                    }
                    tmp->len = new_len;
                    tmp->buf[new_len] = 0;
                    intrusive_ptr_release(pchunk);
                    pchunk = tmp;
                } else {
                    std::memmove(pchunk->buf + pchunk->len, s, sizeof(char_type) * count);
                    pchunk->len = new_len;
                    pchunk->buf[new_len] = 0;
                }
            }
        }
        return *this;
    }

    /// append null-terminated string
    string& append(const char_type* s) noexcept {
        if (!s || !*s) {
            return *this;
        }
        size_type count = str_len<char_type, size_type>(s);
        if (count) {
            append(s, count);
        }
        return *this;
    }

    /// append another string
    string& append(const ro_string_type& s) noexcept {
        if (!s.empty()) {
            return append(s.pchunk->buf, s.pchunk->len);
        }
        return *this;
    }

    /// append fragment of another string
    string& append(const ro_string_type& s, size_type offset, size_type count) noexcept {
        if (!s.empty() && offset < s.pchunk->len) {
            if (offset + count > s.pchunk->len) {
                count = s.pchunk->len - offset;
            }
            return append(s.pchunk->buf + offset, count);
        }
        return *this;
    }

    /// append one or more characters
    string& append(size_type count, char_type c) noexcept {
        if (pchunk) {
            size_type new_len = pchunk->len + count;
            if (pchunk->isShared() || pchunk->size < new_len) {
                chunk_t * tmp = chunk_t::alloc(pchunk->buf, pchunk->len, new_len);
                for (size_type i = 0; i < count; i++) {
                    tmp->buf[pchunk->len + i] = c;
                }
                tmp->len = new_len;
                tmp->buf[new_len] = 0;
                intrusive_ptr_release(pchunk);
                pchunk = tmp;
            } else {
                // own buffer with enough capacity
                for (size_type i = 0; i < count; i++) {
                    pchunk->buf[pchunk->len + i] = c;
                }
                pchunk->len = new_len;
                pchunk->buf[new_len] = 0;
            }
        } else {
            // appending to empty string
            pchunk = chunk_t::alloc(count);
            for (size_type i = 0; i < count; i++) {
                pchunk->buf[i] = c;
            }
            pchunk->len = count;
            pchunk->buf[count] = 0;
        }
        return *this;
    }

    /// append single character
    string& operator << (char_type ch) noexcept { return append(1, ch); }
    /// append C-string
    string& operator << (const char_type * str) noexcept { return append(str); }
    /// append string
    string& operator << (const string & str) noexcept { return append(str); }
    /// append decimal number
    string& operator << (const fmt::decimal v) noexcept { return appendDecimal(v.get()); }
    /// append hex number
    string& operator << (const fmt::hex v) noexcept { return appendHex(v.get()); }

    /// append single character
    string& operator += (char_type ch) noexcept { return append(1, ch); }
    /// append C-string
    string& operator += (const char_type * str) noexcept { return append(str); }
    /// append string
    string& operator += (const string & str) noexcept { return append(str); }
    /// append decimal number
    string& operator += (const fmt::decimal v) noexcept { return appendDecimal(v.get()); }
    /// append hex number
    string& operator += (const fmt::hex v) noexcept { return appendHex(v.get()); }

    /// insert from char* and size
    string& insert(size_type pos, const char_type* s, size_type count) noexcept {
        if (!count) {
            return *this;
        }
        if (!pchunk) {
            // insert into empty string
            pchunk = chunk_t::alloc(s, count, count);
        } else {
            if (pos > pchunk->len) {
                pos = pchunk->len;
            }
            size_type new_len = pchunk->len + count;
            size_type tail_len = pchunk->len - pos;
            if (pchunk->isShared() || new_len > pchunk->size) {
                // create copy
                chunk_t* tmp = chunk_t::alloc(new_len);
                if (pos > 0) {
                    // copy head
                    std::memcpy(tmp->buf, pchunk->buf, sizeof(char_type) * pos);
                }
                // copy inserted content
                std::memcpy(tmp->buf + pos, s, sizeof(char_type) * count);
                // copy tail if needed
                if (tail_len) {
                    // copy tail
                    std::memcpy(tmp->buf + pos + count, pchunk->buf + pos, sizeof(char_type) * tail_len);
                }
                tmp->len = new_len;
                intrusive_ptr_release(pchunk);
                pchunk = tmp;
            } else {
                // insert in-place
                if (tail_len) {
                    // move tail by count chars
                    std::memmove(pchunk->buf + pos + count, pchunk->buf + pos, sizeof(char_type) * tail_len);
                }
                // copy inserted content
                std::memcpy(pchunk->buf + pos, s, sizeof(char_type) * count);
            }
            pchunk->len = new_len;
            pchunk->buf[new_len] = 0;
        }
        return *this;
    }

    /// insert from null-terminated cont char*
    string& insert(size_type pos, const char_type* s) noexcept {
        if (!s || !*s) {
            // attempt inserting empty string
            return *this;
        }
        return insert(pos, s, str_len<char_type, size_type>(s));
    }

    /// insert from string (no self-insert protection)
    string& insert(size_type pos, const ro_string_type& s) noexcept {
        if (s.empty()) {
            // attempt inserting empty string
            return *this;
        }
        return insert(pos, s.pchunk->buf, s.pchunk->len);
    }

    /// insert one or more characters
    string& insert(size_type pos, size_type count, char_type c) noexcept {
        if (!count) {
            return *this;
        }
        if (!pchunk) {
            // insert into empty string
            pchunk = chunk_t::alloc(count);
            for (size_type i = 0; i < count; i++) {
                pchunk->buf[i] = c;
            }
            pchunk->len = count;
            pchunk->buf[count] = 0;
        } else {
            if (pos > pchunk->len) {
                pos = pchunk->len;
            }
            size_type new_len = pchunk->len + count;
            size_type tail_len = pchunk->len - pos;
            if (pchunk->isShared() || new_len > pchunk->size) {
                // create copy
                chunk_t* tmp = chunk_t::alloc(new_len);
                if (pos > 0) {
                    // copy head
                    std::memcpy(tmp->buf, pchunk->buf, sizeof(char_type) * pos);
                }
                // copy inserted content
                for (size_type i = 0; i < count; i++) {
                    tmp->buf[pos + i] = c;
                }
                // copy tail if needed
                if (tail_len) {
                    // copy tail
                    std::memcpy(tmp->buf + pos + count, pchunk->buf + pos, sizeof(char_type) * tail_len);
                }
                tmp->len = new_len;
                intrusive_ptr_release(pchunk);
                pchunk = tmp;
            } else {
                // insert in-place
                if (tail_len) {
                    // move tail by count chars
                    std::memmove(pchunk->buf + pos + count, pchunk->buf + pos, sizeof(char_type) * tail_len);
                }
                // copy inserted content
                for (size_type i = 0; i < count; i++) {
                    pchunk->buf[pos + i] = c;
                }
            }
            pchunk->len = new_len;
            pchunk->buf[new_len] = 0;
        }
        return *this;
    }

    /// compact buffer if possible -- free unused buffer space
    string& pack() noexcept {
        if (pchunk) {
            size_type new_size = chunk_t::alignSize(pchunk->len);
            if (pchunk->size > new_size) {
                chunk_t * tmp = chunk_t::alloc(pchunk->buf, pchunk->len, pchunk->len);
                intrusive_ptr_release(pchunk);
                pchunk = tmp;
            }
        }
        return *this;
    }

    /// replace part of string with count chars from char*
    string& replace(size_type pos, size_type n, const char_type * s, size_type count) noexcept {
        if (!count) {
            return *this;
        }
        if (empty()) {
            return assign(s, count);
        } else {
            // need to modify
            size_type old_len = pchunk->len;
            if (pos > old_len) {
                pos = old_len;
            }
            if (pos + n > old_len) {
                n = old_len - pos;
            }
            if (!n) {
                // delegate to insert
                return insert(pos, s, count);
            }
            if (pos == old_len) {
                // use append
                return append(s, count);
            }
            size_type new_len = old_len + count - n;
            size_type tail_len = old_len - (pos + n);
            if (new_len > pchunk->size || pchunk->isShared()) {
                // need to create copy -- no space or has other refs
                chunk_t * tmp = chunk_t::alloc(pchunk->buf, new_len, new_len);
                // move tail to the end of buffer
                std::memmove(tmp->buf + new_len - tail_len, tmp->buf + old_len - tail_len, sizeof(char_type) * tail_len);
                // fill gap with inserted string
                std::memcpy(tmp->buf + pos, s, sizeof(char_type) * count);
                // replace this string with new
                intrusive_ptr_release(pchunk);
                pchunk = tmp;
            } else {
                // may edit in-place
                // move tail to the end of buffer
                std::memmove(pchunk->buf + new_len - tail_len, pchunk->buf + old_len - tail_len, sizeof(char_type) * tail_len);
                // fill gap with inserted string
                std::memcpy(pchunk->buf + pos, s, sizeof(char_type) * count);
            }
            // update length
            pchunk->len = new_len;
            pchunk->buf[new_len] = 0;
            return *this;
        }
    }

    /// replace part of string with count chars from char*
    string& replace(size_type pos, size_type n, size_type count, char_type c) noexcept {
        if (!count) {
            // nothing to insert
            return *this;
        }
        if (empty()) {
            return append(count, c);
        } else {
            // need to modify
            size_type old_len = pchunk->len;
            if (pos > old_len) {
                pos = old_len;
            }
            if (pos + n > old_len) {
                n = old_len - pos;
            }
            if (!n) {
                // delegate to insert
                return insert(pos, count, c);
            }
            if (pos == old_len) {
                // use append
                return append(count, c);
            }
            size_type new_len = old_len + count - n;
            size_type tail_len = old_len - (pos + n);
            if (new_len > pchunk->size || pchunk->isShared()) {
                // need to create copy -- no space or has other refs
                chunk_t * tmp = chunk_t::alloc(pchunk->buf, new_len, new_len);
                // move tail to the end of buffer
                std::memmove(tmp->buf + new_len - tail_len, tmp->buf + old_len - tail_len, sizeof(char_type) * tail_len);
                // fill gap with inserted string
                for (size_type i = 0; i < count; i++) {
                    tmp->buf[pos + i] = c;
                }
                // replace this string with new
                intrusive_ptr_release(pchunk);
                pchunk = tmp;
            } else {
                // may edit in-place
                // move tail to the end of buffer
                std::memmove(pchunk->buf + new_len - tail_len, pchunk->buf + old_len - tail_len, sizeof(char_type) * tail_len);
                // fill gap with inserted string
                for (size_type i = 0; i < count; i++) {
                    pchunk->buf[pos + i] = c;
                }
            }
            // update length
            pchunk->len = new_len;
            pchunk->buf[new_len] = 0;
            return *this;
        }
    }

    /// replace part of string with count chars from char*
    string& replace(size_type pos, size_type n, const char_type * s) noexcept {
        if (!s || !*s) {
            // nothing to insert
            return *this;
        }
        size_type count = str_len<char_type, size_type>(s);
        return replace(pos, n, s, count);
    }

    /// replace part of string with count chars from char*
    string& replace(size_type pos, size_type n, const ro_string_type& s) noexcept {
        if (s.empty()) {
            // nothing to insert
            return *this;
        }
        return replace(pos, n, s.pchunk->buf, s.pchunk->len);
    }

    /// replace part of string with count chars from char*
    string& replace(size_type pos, size_type n, const ro_string_type& s, size_type offset, size_type count) noexcept {
        if (s.empty()) {
            // nothing to insert
            return *this;
        }
        size_type s_len = s.pchunk->len;
        if (offset >= s_len) {
            // source data indexes outside source string bounds
            return *this;
        }
        // ensure source range is inside string - truncate if needed
        if (offset + count > s_len) {
            count = s_len - offset;
        }
        return replace(pos, n, s.pchunk->buf + offset, count);
    }

    /// replace all occurences of character replaceWhat with character replaceTo
    string& replace(char_type replaceWhat, char_type replaceTo) noexcept {
        if (empty()) {
            // nothing to replace
            return *this;
        }
        size_type len = pchunk->len;
        char_type * buf = pchunk->buf;
        for (size_type i = 0; i < len; i++) {
            if (buf[i] == replaceWhat) {
                // first match found: replace is needed
                if (pchunk->isOwn()) {
                    // own buffer : replace in-place
                    buf[i++] = replaceTo;
                    while (i < len) {
                        if (buf[i] == replaceWhat) {
                            buf[i] = replaceTo;
                        }
                        i++;
                    }
                } else {
                    // need to create a copy
                    chunk_t * tmp = chunk_t::alloc(pchunk->buf, pchunk->len, pchunk->len);
                    // switch to new buffer
                    buf = tmp->buf;
                    // replace first char
                    buf[i++] = replaceTo;
                    while (i < len) {
                        if (buf[i] == replaceWhat) {
                            buf[i] = replaceTo;
                        }
                        i++;
                    }
                    intrusive_ptr_release(pchunk);
                    pchunk = tmp;
                }
                break;
            }
        }
        return *this;
    }

    /// return C-style null-terminated string pointer
    const char_type * data() const noexcept {
        return c_str();
    }

    /// return modifable C-style null-terminated string pointer; for empty string returns pointer to fake empty z-string buffer
    char_type * data() noexcept {
        if (pchunk == nullptr) {
            return reinterpret_cast<char_type *>(&fake_null_buffer_32);
        } else {
            lock(pchunk->len);
            // enforce null-termination
            const_cast<chunk_t*>(pchunk)->buf[pchunk->len] = 0;
            return pchunk->buf;
        }
    }

    /// ensures that reference count is 1; if string is null or we own it, do nothing
    /// only used from data() ? what is a better place for it?
    void lock( size_type newsize ) noexcept {
        if (pchunk) {
            // string is not null
            if (pchunk->isShared()) {
                // if we don't own string, make a copy
                if (newsize < pchunk->len) {
                    // ensure all chars from old string were copied
                    newsize = pchunk->len;
                }
                chunk_t * tmp = chunk_t::alloc(newsize);
                tmp->len = pchunk->len;
                std::memcpy(tmp->buf, pchunk->buf, tmp->len);
                tmp->buf[tmp->len] = 0;
                intrusive_ptr_release(pchunk);
                pchunk = tmp;
            }
        }
    }



    /// returns pointer to modifable string buffer
    char_type * modify() noexcept {
        if (!pchunk) {
            // allocate small buffer
            pchunk = chunk_t::alloc(8);
        } else {
            // non-empty string
            // string is not null
            if (pchunk->isShared()) {
                chunk_t * tmp = chunk_t::alloc(pchunk->len);
                tmp->len = pchunk->len;
                std::memcpy(tmp->buf, pchunk->buf, tmp->len);
                tmp->buf[tmp->len] = 0;
                intrusive_ptr_release(pchunk);
                pchunk = tmp;
            }
        }
        return pchunk->buf;
    }

    /// returns substring starting with position pos, and specified count of chars (pass npos to copy till end of string)
    string substr(size_type pos, size_type count = npos) const noexcept {
        string tmp;
        if (!pchunk || pos >= pchunk->len) {
            return tmp;
        }
        size_type sz = pchunk->len - pos;
        if (count > sz) {
            count = sz;
        }
        tmp.pchunk = chunk_t::alloc(pchunk->buf + pos, count, count);
        return tmp;
    }

    /// swaps content of two strings, by swapping of pointers
    void swap(string & s) {
        chunk_t * tmp = pchunk;
        pchunk = s.pchunk;
        s.pchunk = tmp;
    }

    /// constructs string representation of integer
    static string itoa( int n ) {
        char_type buf[16];
        size_type i=0;
        bool negative = false;
        if (n==0) {
            buf[i++] = '0';
            //cs8("0");
        } else {
            if (n<0) {
                negative = true;
                n = -n;
            }
            for ( ; n; n/=10 ) {
                buf[i++] = '0' + (n%10);
            }
        }
        if (negative)
            buf[i++] = '-';
        // reverse string
        for (size_type j = 0; j < i/2; j++) {
            char_type tmp = buf[j];
            buf[j] = buf[i - 1 - j];
            buf[i - 1 - j] = tmp;
        }
        return string(buf, i);
    }
    /// constructs string representation of unsigned integer
    static string itoa( unsigned int n ) {
        char_type buf[16];
        size_type i=0;
        if (n==0) {
            buf[i++] = '0';
            //cs8("0");
        } else {
            for ( ; n; n/=10 ) {
                buf[i++] = '0' + (n%10);
            }
        }
        // reverse string
        for (size_type j = 0; j < i/2; j++) {
            char_type tmp = buf[j];
            buf[j] = buf[i - 1 - j];
            buf[i - 1 - j] = tmp;
        }
        return string(buf, i);
    }
    // constructs string representation of 64 bit integer
    static string itoa( lInt64 n ) {
        char_type buf[24];
        size_type i=0;
        bool negative = false;
        if (n==0) {
            buf[i++] = '0';
            //cs8("0");
        } else {
            if (n<0) {
                negative = true;
                n = -n;
            }
            for ( ; n; n/=10 ) {
                buf[i++] = '0' + (n%10);
            }
        }
        if (negative)
            buf[i++] = '-';
        // reverse string
        for (size_type j = 0; j < i/2; j++) {
            char_type tmp = buf[j];
            buf[j] = buf[i - 1 - j];
            buf[i - 1 - j] = tmp;
        }
        return string(buf, i);
    }


private:
    //chunk_t * pchunk {nullptr};
};

// template<typename char_type, typename refcounter_type>
// const string<char_type, refcounter_type> string<char_type, refcounter_type>::empty_str{};

typedef lstring_chunk_t<lChar8, lUInt32, std::atomic_int> lstring8_chunk_t;
typedef lstring_chunk_t<lChar16, lUInt32, std::atomic_int> lstring16_chunk_t;
typedef lstring_chunk_t<lChar32, lUInt32, std::atomic_int> lstring32_chunk_t;

typedef string<lChar8, std::atomic_int> lString8;
typedef string<lChar16, std::atomic_int> lString16;
typedef string<lChar32, std::atomic_int> lString32;


}


#endif // LVSTRING2_H
