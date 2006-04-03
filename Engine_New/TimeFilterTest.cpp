
// Base
#include <epicsThread.h>
// Tools
#include <UnitTest.h>
#include <AutoPtr.h>
#include <epicsTimeHelper.h>
// Local

#include "TimeFilter.h"

class MyPVListener : public ProcessVariableListener
{
public:
    bool connected;
    size_t values;
    
    MyPVListener() : connected(false), values(0)
    {}
    
    void pvConnected(Guard &guard, ProcessVariable &pv,
                     const epicsTime &when)
    {
        printf("ProcessVariableListener PV: '%s' connected!\n",
               pv.getName().c_str());
        connected = true;
    }
    
    void pvDisconnected(Guard &guard, ProcessVariable &pv,
                        const epicsTime &when)
    {
        connected = false;
    }
    
    void pvValue(class Guard &guard, class ProcessVariable &pv,
                 const RawValue::Data *data)
    {
        ++values;
    }
};

TEST_CASE test_time_filter()
{
    WritableEngineConfig config;
    config.setIgnoredFutureSecs(60);
    
    ProcessVariableContext ctx;
    ProcessVariable pv(ctx, "test");
    MyPVListener pvl;
    TimeFilter filt(config, &pvl);     
    
    // Connect gets passed.
    epicsTime time = epicsTime::getCurrent();   
    TEST(pvl.values == 0);
    {
        Guard guard(pv);
        filt.pvConnected(guard, pv, time);
    }
    TEST(pvl.values == 0);
    TEST(pvl.connected == true);
     
    // Data gets passed.
    DbrType type = DBR_TIME_DOUBLE;
    DbrCount count = 1;
    RawValueAutoPtr value(RawValue::allocate(type, count, 1));
    RawValue::setDouble(type, count, value, 3.14);
    RawValue::setTime(value, time);    
    {
        Guard guard(pv);
        filt.pvValue(guard, pv, value);
    }
    TEST(pvl.values == 1);

    // Same time gets passed.
    {
        Guard guard(pv);
        filt.pvValue(guard, pv, value);
    }
    TEST(pvl.values == 2);

    // New time gets passed.
    time += 10;
    RawValue::setTime(value, time);    
    {
        Guard guard(pv);
        filt.pvValue(guard, pv, value);
    }
    TEST(pvl.values == 3);

    // Back-in-time is blocked
    time -= 20;
    RawValue::setTime(value, time);    
    {
        Guard guard(pv);
        filt.pvValue(guard, pv, value);
    }
    TEST(pvl.values == 3);

    // New time gets passed.
    time += 30;
    RawValue::setTime(value, time);    
    {
        Guard guard(pv);
        filt.pvValue(guard, pv, value);
    }
    TEST(pvl.values == 4);

    // Back-in-time is blocked
    time -= 10;
    RawValue::setTime(value, time);    
    {
        Guard guard(pv);
        filt.pvValue(guard, pv, value);
    }
    TEST(pvl.values == 4);
    
    // New time gets passed.
    time += 20;
    RawValue::setTime(value, time);    
    {
        Guard guard(pv);
        filt.pvValue(guard, pv, value);
    }
    TEST(pvl.values == 5);
    
    // Time is now at 'now + 30'
    // Toggle 'ignored future' test
    time = epicsTime::getCurrent() + (config.getIgnoredFutureSecs() + 10);
    RawValue::setTime(value, time);    
    {
        Guard guard(pv);
        filt.pvValue(guard, pv, value);
    }
    TEST(pvl.values == 5);
    
    // Disconnect gets passed.
    time = epicsTime::getCurrent();   
    {
        Guard guard(pv);
        filt.pvDisconnected(guard, pv, time);
    }
    TEST(pvl.connected == false);

    TEST_OK;
}


