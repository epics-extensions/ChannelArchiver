
#include "AutoPtr.h"
#include "UnitTest.h"

class X
{
public:
    static int deletes;
    X() {}

    ~X()
    {
        ++deletes;
    }

    int val;
};

int X::deletes = 0;

TEST_CASE test_autoptr()
{
    {
        AutoPtr<int> i(new int);
        *i = 42;
        TEST(*i == 42);
    }
    {
        AutoPtr<X> x(new X);
        x->val = 42;
        (*x).val = 43;

        AutoPtr<X> x2(x);
        TEST(x2->val == 43);
    }
    {
        X x;
        AutoPtr<X> p(&x);
        p.release();
        p.assign(&x);
        p.release();
    }
    {
        int i;
        for (i=0; i<5; ++i)
        {
            AutoPtr<X> x(new X);
            x->val = i;
        }
    }
    TEST_MSG(X::deletes == 7, "All classes were auto-deleted");
    TEST_OK;
}

