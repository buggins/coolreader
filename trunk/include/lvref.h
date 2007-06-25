/** \file lvref.h
    \brief smart pointer with reference counting template

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.
    See LICENSE file for details.

*/

#ifndef __LVREF_H_INCLUDED__
#define __LVREF_H_INCLUDED__

#include "lvmemman.h"

/// Memory manager pool for ref counting
/**
    For fast and efficient allocation of ref counter structures
*/
#if (LDOM_USE_OWN_MEM_MAN==1)
extern ldomMemManStorage * pmsREF;
#endif

/// Reference counter structure
/**
    For internal usage in LVRef<> class
*/
class ref_count_rec_t {
public:
    int _refcount;
    void * _obj;
    static ref_count_rec_t null_ref;

    ref_count_rec_t( void * obj ) : _refcount(1), _obj(obj) { }
#if (LDOM_USE_OWN_MEM_MAN==1)
    void * operator new( size_t size )
    {
        if (pmsREF == NULL)
        {
            pmsREF = new ldomMemManStorage(sizeof(ref_count_rec_t));
        }
        return pmsREF->alloc();
    }
    void operator delete( void * p )
    {
        pmsREF->free((ldomMemBlock *)p);
    }
#endif
};

/// Smart pointer with reference counting
/**
    Stores pointer to object and reference counter.
    Imitates usual pointer behavior, but deletes object 
    when there are no more references on it.
    On copy, increases reference counter
    On destroy, decreases reference counter; deletes object if counter became 0
    \param T class of stored object
*/
template <class T> class LVRef
{
private:
    ref_count_rec_t * _ptr;
    //========================================
    ref_count_rec_t * AddRef() const { ++_ptr->_refcount; return _ptr; }
    //========================================
    void Release()
    { 
        if (--_ptr->_refcount == 0) 
        {
            if ( _ptr->_obj )
                delete (reinterpret_cast<T*>(_ptr->_obj));
            delete _ptr;
        }
    }
    //========================================
public:

    /// Default constructor.
    /** Initializes pointer to NULL */
    LVRef() : _ptr(&ref_count_rec_t::null_ref) { ref_count_rec_t::null_ref._refcount++; }

    /// Constructor by object pointer.
    /** Initializes pointer to given value 
        \param ptr is a pointer to object
    */
    explicit LVRef( T * ptr ) {
        if (ptr)
        {
            _ptr = new ref_count_rec_t(ptr);
        }
        else
        {
            ref_count_rec_t::null_ref._refcount++;
            _ptr = &ref_count_rec_t::null_ref;
        }
    }

    /// Copy constructor.
    /** Creates copy of object pointer. Increments reference counter instead of real copy.
        \param ref is reference to copy
    */
    LVRef( const LVRef & ref ) { _ptr = ref.AddRef(); }

    /// Destructor.
    /** Decrements reference counter; deletes object if counter became 0. */
    ~LVRef() { Release(); }

    /// Clears pointer.
    /** Sets object pointer to NULL. */
    void Clear() { Release(); _ptr = &ref_count_rec_t::null_ref; ++_ptr->_refcount; }

    /// Copy operator.
    /** Duplicates a pointer from specified reference. 
        Increments counter instead of copying of object. 
        \param ref is reference to copy
    */
    LVRef & operator = ( const LVRef & ref )
    {
        if (!ref._ptr->_obj)
        {
            Clear();
        }
        else
        {
            if (_ptr!=ref._ptr)
            {
                Release();
                _ptr = ref.AddRef(); 
            }
        }
        return *this;
    }

    /// Object pointer assignment operator.
    /** Sets object pointer to the specified value. 
        Reference counter is being initialized to 1.
        \param obj pointer to object
    */
    LVRef & operator = ( T * obj )
    {
        if ( !obj )
        {
            Clear();
        }
        else
        {
            if (_ptr->_obj!=obj)
            {
                Release();
                _ptr = new ref_count_rec_t(obj);
            }
        }
        return *this;
    }

    /// Returns stored pointer to object.
    /** Imitates usual pointer behavior. 
        Usual way to access object fields. 
    */
    T * operator -> () const { return reinterpret_cast<T*>(_ptr->_obj); }

    /// Dereferences pointer to object.
    /** Imitates usual pointer behavior. */
    T & operator * () const { return *(reinterpret_cast<T*>(_ptr->_obj)); }

    /// To check reference counter value.
    /** It might be useful in some cases. 
        \return reference counter value.
    */
    int getRefCount() const { return _ptr->_refcount; }

    /// Returns stored pointer to object.
    /** Usual way to get pointer value. 
        \return stored pointer to object.
    */
    T * get() const { return reinterpret_cast<T*>(_ptr->_obj); }

    /// Checks whether pointer is NULL or not.
    /** \return true if pointer is NULL.
        \sa isNull() */
    bool operator ! () const { return !_ptr->_obj; }

    /// Checks whether pointer is NULL or not.
    /** \return true if pointer is NULL. 
        \sa operator !()
    */
    bool isNull() const { return _ptr->_obj == NULL; }
};


#endif
