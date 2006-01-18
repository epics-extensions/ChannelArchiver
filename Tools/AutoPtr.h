// -*- c++ -*-

#ifndef _AUTO_PTR_H_
#define _AUTO_PTR_H_

// System
#include <stdlib.h> // size_t

/// \ingroup Tools

/// AutoPtr: Holds pointer, deletes in destructor.
///
/// This AutoPtr is meant for holding a pointer to
/// one instance of something.
/// The instance is removed via delete.
///
/// For arrays that need to be removed via delete[],
/// see AutoArrayPtr.
template<class T>class AutoPtr
{
public:
    AutoPtr() : ptr(0) {};

    /// Assign pointer to AutoPtr.
    AutoPtr(T *in) : ptr(in) {};

    /// Copying from other AutoPtr causes rhs to release ownership.
    AutoPtr(AutoPtr &rhs)
    {
        ptr = rhs.release();
    }

    /// Destructor deletes owned pointer.
    ~AutoPtr()
    {
        assign(0);
    }

    /// Copying from other AutoPtr causes rhs to release ownership.
    AutoPtr & operator = (AutoPtr &rhs)
    {
        assign(rhs.release());
        return *this;
    }

    AutoPtr & operator = (T *p)
    {
        assign(p);
        return *this;
    }
    
    operator bool () const
    {
        return ptr != 0;
    }
    
    /// Allow access just like ordinary pointer.
    T &operator *() const
    {
        return *ptr;
    }

    /// Allow access just like ordinary pointer.
    T *operator ->() const
    {
        return ptr;
    }

    /// Allow access just like ordinary pointer.
    operator T * () const
    {
        return ptr;
    }

    /// Assign a new pointer, deleting existing one.
    void assign(T *new_ptr)
    {
        if (ptr)
            delete ptr;
        ptr = new_ptr;
    }

    /// Release ownership.
    ///
    /// The AutoPtr is set to 0, and the pointer that
    /// used to be handled by the AutoPtr is returned.
    ///
    /// @return Returns the original value.
    T * release()
    {
        T *tmp = ptr;
        ptr = 0;
        return tmp;
    }
    
private:
    T *ptr;
};

template<class T>class AutoArrayPtr
{
public:
    AutoArrayPtr() : arr(0) {};

    /// Assign pointer to AutoArrayPtr.
    AutoArrayPtr(T *in) : arr(in) {};

    /// Copying from other AutoArrayPtr causes rhs to release ownership.
    AutoArrayPtr(AutoArrayPtr &rhs)
    {
        arr = rhs.release();
    }

    /// Destructor deletes owned pointer.
    ~AutoArrayPtr()
    {
        assign(0);
    }

    /// Copying from other AutoArrayPtr causes rhs to release ownership.
    AutoArrayPtr & operator = (AutoArrayPtr &rhs)
    {
        assign(rhs.release());
        return *this;
    }

    AutoArrayPtr & operator = (T *p)
    {
        assign(p);
        return *this;
    }
    
    operator bool () const
    {
        return arr != 0;
    }
    
    /// Access one array element.
    T & operator [] (size_t i) const
    {
        return arr[i];
    }

    /// Assign a new pointer, deleting existing one.
    void assign(T *new_arr)
    {
        if (arr)
            delete [] arr;
        arr = new_arr;
    }

    /// Release ownership.
    ///
    /// The AutoArrayPtr is set to 0, and the pointer that
    /// used to be handled by the AutoArrayPtr is returned.
    ///
    /// @return Returns the original value.
    T * release()
    {
        T *tmp = arr;
        arr = 0;
        return tmp;
    }
    
private:
    T *arr;
};

#endif
