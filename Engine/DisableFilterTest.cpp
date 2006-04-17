
// Base
#include <epicsThread.h>
// Tools
#include <UnitTest.h>
#include <AutoPtr.h>
#include <epicsTimeHelper.h>
// Local

#include "DisableFilter.h"

class DAFPVListner : public ProcessVariableListener
{
public:
    bool connected;
    size_t values;
    
    DAFPVListner() : connected(false), values(0)
    {}
    
    void pvConnected(Guard &guard, ProcessVariable &pv,
                     const epicsTime &when)
    {
        printf("ProcessVariableListener PV: '%s' connected.\n",
               pv.getName().c_str());
        connected = true;
    }
    
    void pvDisconnected(Guard &guard, ProcessVariable &pv,
                        const epicsTime &when)
    {
        printf("ProcessVariableListener PV: '%s' disconnected.\n",
               pv.getName().c_str());
        connected = false;
    }
    
    void pvValue(class Guard &guard, class ProcessVariable &pv,
                 const RawValue::Data *data)
    {
        printf("ProcessVariableListener PV: '%s' value.\n",
               pv.getName().c_str());
        ++values;
    }
};

TEST_CASE test_disable_filter()
{
    ProcessVariableContext ctx;
    ProcessVariable pv(ctx, "test");
    DAFPVListner pvl;
    DisableFilter filt(&pvl);     
    
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
    time += 1; 
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
    
    // Disconnect gets passed.
    time += 1;   
    {
        Guard guard(pv);
        filt.pvDisconnected(guard, pv, time);
    }
    TEST(pvl.connected == false);

    // Disable
    puts (" --- Disable ---");
    {
        Guard guard(pv);
        filt.disable(guard);
    }

    // Re-connect
    {
        Guard guard(pv);
        filt.pvConnected(guard, pv, time);
    }
    TEST(pvl.values == 1);
    TEST(pvl.connected == true);
    
    // Sample doesn't pass.
    time += 1; 
    RawValue::setTime(value, time);    
    {
        Guard guard(pv);
        filt.pvValue(guard, pv, value);
    }
    TEST(pvl.values == 1);
    
    // Enable again.
    puts (" --- Enable ---");
    time += 1;
    {
        Guard guard(pv);
        filt.enable(guard, pv, time);
    }
    // Still connected
    TEST(pvl.connected == true);
    // And the last value
    TEST(pvl.values == 2);

    TEST_OK;
}


