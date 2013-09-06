#ifndef LVAUTOPTR_H
#define LVAUTOPTR_H

/// auto pointer
template <class T >
class LVAutoPtr {
    T * p;
    LVAutoPtr( const LVAutoPtr & v ) { } // no copy allowed
public:
    LVAutoPtr()
        : p(NULL)
{
    }
    explicit LVAutoPtr( T* ptr )
        : p(ptr)
    {
    }
    bool isNull() const {
        return p == NULL;
    }
    bool operator !() const { return p == NULL; }
    inline void clear()
    {
        if (p)
            delete p;
        p = NULL;
    }
    ~LVAutoPtr()
    {
        clear();
    }
    inline T * operator -> ()
    {
        return p;
    }
    inline const T * operator -> () const
    {
        return p;
    }
    inline T & operator [] (int index) { return p[index]; }
    inline T * get() const { return p; }
    inline T & operator * ()
    {
        return *p;
    }
    inline const T & operator * () const
    {
        return *p;
    }
    inline LVAutoPtr & operator = ( T* ptr )
    {
        if ( p==ptr )
            return *this;
        if ( p )
            delete p;
        p = ptr;
        return *this;
    }
};


#endif // LVAUTOPTR_H
