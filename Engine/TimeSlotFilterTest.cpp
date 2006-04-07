
// Base
#include <epicsThread.h>
// Tools
#include <UnitTest.h>
#include <AutoPtr.h>
#include <epicsTimeHelper.h>
// Local

#include "TimeSlotFilter.h"

class TSFTPVListner : public ProcessVariableListener
{
public:
    bool connected;
    size_t values;
    
    TSFTPVListner() : connected(false), values(0)
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

TEST_CASE test_time_slot_filter()
{
    WritableEngineConfig config;
    config.setIgnoredFutureSecs(60);
    
    ProcessVariableContext ctx;
    ProcessVariable pv(ctx, "test");
    TSFTPVListner pvl;
    TimeSlotFilter filt(10.0, &pvl);     
    
    // Connect gets passed.
    epicsTime time = epicsTime::getCurrent();   
    TEST(pvl.values == 0);
    {
        Guard guard(pv);
        filt.pvConnected(guard, pv, time);
    }
    TEST(pvl.values == 0);
    TEST(pvl.connected == true);
     
    // Initial sample gets passed.
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
    
    // This is the next time slot, so this value should pass, too.
    time = roundTimeUp(time, 10.0); 
    RawValue::setTime(value, time);    
    {
        Guard guard(pv);
        filt.pvValue(guard, pv, value);
    }
    TEST(pvl.values == 2);

    // But not again...
    int i;
    for (i=1; i<10; ++i)
    {
        RawValue::setTime(value, time + 0.1*i);    
        {
            Guard guard(pv);
            filt.pvValue(guard, pv, value);
        }
        TEST(pvl.values == 2);
    }
    // ... until the next time slot is reached
    time += 10.0;
    RawValue::setTime(value, time);    
    {
        Guard guard(pv);
        filt.pvValue(guard, pv, value);
    }
    TEST(pvl.values == 3);

    // One more try
    for (i=1; i<10; ++i)
    {
        RawValue::setTime(value, time + 0.1*i);    
        {
            Guard guard(pv);
            filt.pvValue(guard, pv, value);
        }
        TEST(pvl.values == 3);
    }
    // ... until the next time slot is reached
    time += 30.0;
    RawValue::setTime(value, time);    
    {
        Guard guard(pv);
        filt.pvValue(guard, pv, value);
    }
    TEST(pvl.values == 4);


    // Disconnect gets passed.
    time = epicsTime::getCurrent();   
    {
        Guard guard(pv);
        filt.pvDisconnected(guard, pv, time);
    }
    TEST(pvl.connected == false);

    TEST_OK;
}


