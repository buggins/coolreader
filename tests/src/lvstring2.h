#ifndef LVSTRING2_H
#define LVSTRING2_H

#include "lvtypes.h"
#include <atomic>
#include <stdlib.h>
#include <cstdlib>
#include <cstring>

namespace lv {

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
#define LS_COUNT_ALLOC ls_alloc_stats.allocCount++
#define LS_COUNT_FREE ls_alloc_stats.freeCount++
#define LS_COUNT_COPY_CONSTR ls_alloc_stats.copyConstr++
#define LS_COUNT_MOVE_CONSTR ls_alloc_stats.moveConstr++
#define LS_COUNT_COPY_ASSIGN ls_alloc_stats.copyAssign++
#define LS_COUNT_MOVE_ASSIGN ls_alloc_stats.moveAssign++

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

// String data buffer with ref count

// forward declaration of string class
template<typename char_type, typename size_type, typename refcounter_type>
class string;

template <typename char_type, typename size_type, typename refcounter_type = std::atomic_int>
struct lstring_chunk_t {

    // friend class lString8;
    // friend class lString32;
    // friend struct lstring_chunk_slice_t;
    friend class string<char_type, size_type, refcounter_type>;

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
        return refCount;
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
        LS_COUNT_ALLOC;
        //lstring_chunk_t * res = static_cast<lstring_chunk_t *>( ::malloc(sizeof(lstring_chunk_t) + sizeof(char_type) * (sz + 1)) );
        //lstring_chunk_t * res = static_cast<lstring_chunk_t *>( ::malloc(sizeof(lstring_chunk_t) + sizeof(char_type) * (sz + 1)) );
        sz = alignSize(sz);
        size_t allocBytes = (sizeof(lstring_chunk_t) + sizeof(char_type) * (sz + 1));
        lstring_chunk_t * res = static_cast<lstring_chunk_t *>( std::aligned_alloc(alloc_align_bytes, allocBytes) );
        res->size = sz;
        res->len = 0;
        res->buf[0] = 0;
        res->buf[sz] = 0;
        res->refCount = 1;
        return res;
    }

    /// chunk allocation function: create string buffer initialized with count chars from string s with reserved sz characters + zero termination char, ref counter 1
    static lstring_chunk_t * alloc(const char_type * s, size_type count, size_type sz = 0) noexcept {
        LS_COUNT_ALLOC;
        if (sz < count)
            sz = count;
        sz = alignSize(sz);
        //lstring_chunk_t * res = static_cast<lstring_chunk_t *>( ::malloc(sizeof(lstring_chunk_t) + sizeof(char_type) * (sz + 1)) );
        size_t allocBytes = (sizeof(lstring_chunk_t) + sizeof(char_type) * (sz + 1));
        lstring_chunk_t * res = static_cast<lstring_chunk_t *>( std::aligned_alloc(alloc_align_bytes, allocBytes) );
        std::memcpy(res->buf, s, count * sizeof(char_type));
        res->buf[count] = 0;
        res->buf[sz] = 0;
        res->size = sz;
        res->len = count;
        res->refCount = 1;
        return res;
    }

    /// free chunk memory
    static void free( const lstring_chunk_t * pChunk ) noexcept {
        LS_COUNT_FREE;
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

template <typename char_type, typename size_type, typename refcounter_type = std::atomic_int>
class string {
    using chunk_t = lstring_chunk_t<char_type, size_type, refcounter_type>;

public:
    string() noexcept = default;
    explicit string(const char_type* s, size_type count, size_type reserved) noexcept {
        if (count == 0 || s == nullptr || s[0] == 0) {
            pchunk = nullptr;
        } else {
            pchunk = chunk_t::alloc(s, count, reserved);
        }
    }
    explicit string(const char_type* s) noexcept {
        pchunk = chunk_t::alloc(s, str_len<char_type, size_type>(s), 0);
    }
    /// copy constructor
    string(const string&s) noexcept {
        LS_COUNT_COPY_CONSTR;
        pchunk = s.pchunk;
        intrusive_ptr_add_ref(pchunk);
    }
    /// move constructor
    string(string&&s ) noexcept {
        LS_COUNT_MOVE_CONSTR;
        pchunk = s.pchunk;
        s.pchunk = nullptr;
    }
    ~string() noexcept {
        if (pchunk != nullptr) {
            intrusive_ptr_release(pchunk);
        }
    }
    /// move assignment
    string& operator = (string&& s) noexcept {
        LS_COUNT_MOVE_ASSIGN;
        if (pchunk != nullptr) {
            intrusive_ptr_release(pchunk);
        }
        pchunk = s.pchunk;
        s.pchunk = nullptr;
        return *this;
    }
    /// copy assignment
    string& operator = (const string& s) noexcept {
        LS_COUNT_COPY_ASSIGN;
        if (&s == this) {
            // safe self assignment: do nothing
            return *this;
        }
        if (pchunk != nullptr) {
            // ignore self-assignment
            if (pchunk != s.pchunk) {
                intrusive_ptr_release(pchunk);
                if (pchunk) {
                    // assigned non-empty string
                    intrusive_ptr_add_ref(pchunk);
                }
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

    /// sets value to null string, freeing buffer
    void clear() noexcept {
        if (pchunk != nullptr) {
            intrusive_ptr_release(pchunk);
            pchunk = nullptr;
        }
    }

    /// resets length to zero, preparing to modification with reserved size
    void reset(size_type size) noexcept {
        if (pchunk == nullptr) {
            // buffer is allocated and has capacity at least size
            pchunk = chunk_t::alloc(size);
        } else {
            // this string is non-empty
            if (pchunk->getRefCount() == 1 && pchunk->size >= size) {
                // this is already our own string with enough capacity -- just reset length
                pchunk->len = 0;
            } else {
                // this string has not enough capacity for size, clear and alloc new
                intrusive_ptr_release(pchunk);
                pchunk = chunk_t::alloc(size);
            }
        }
    }

    /// resize and make editable, presercing current content, preparing to modification with reserved size
    void resize(size_type size, char_type e = 0) noexcept {
        if (pchunk == nullptr) {
            // buffer is allocated and has capacity at least size
            pchunk = chunk_t::alloc(size);
        } else {
            // this string is non-empty
            if (pchunk->getRefCount() == 1 && pchunk->size >= size) {
                // this is already our own string with enough capacity -- do nothing
            } else {
                // create new object and copy existing data
                chunk_t * tmp = chunk_t::alloc(pchunk->buf, pchunk->len, size);
                tmp->buf[tmp->len] = 0; // zterm
                // release old chunk and assign new one
                intrusive_ptr_release(pchunk);
                pchunk = tmp;
            }
        }
    }

    /// returns true if string is empty
    bool empty() const noexcept { return pchunk == nullptr || pchunk->len==0; }
    /// returns character count
    size_type   length() const noexcept { return pchunk == nullptr ? 0 : pchunk->len; }
    /// returns buffer size
    size_type   size() const noexcept { return capacity(); }
    /// changes buffer size
    //void  resize(size_type count = 0, value_type e = 0);
    /// returns maximum number of chars that can fit into buffer (there is always additional one char space for trailing 0 which is not counted)
    size_type   capacity() const noexcept { return pchunk==nullptr ? 0 : pchunk->size; }

private:
    chunk_t * pchunk {nullptr};
};

typedef lstring_chunk_t<lChar8, lUInt32, std::atomic_int> lstring8_chunk_t;
typedef lstring_chunk_t<lChar16, lUInt32, std::atomic_int> lstring16_chunk_t;
typedef lstring_chunk_t<lChar32, lUInt32, std::atomic_int> lstring32_chunk_t;

typedef string<lChar8, lUInt32, std::atomic_int> lString8;
typedef string<lChar16, lUInt32, std::atomic_int> lString16;
typedef string<lChar32, lUInt32, std::atomic_int> lString32;

void test_lstring2();

}


#endif // LVSTRING2_H
