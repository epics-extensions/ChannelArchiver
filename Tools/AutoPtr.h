// -*- c++ -*-

#ifndef _AUTO_PTR_H_
#define _AUTO_PTR_H_

/// \ingroup Tools

/// AutoPtr: Holds pointer, deletes in destructor.
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

#endif
