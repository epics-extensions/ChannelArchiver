
// Base
#include <epicsThread.h>
// Tools
#include <UnitTest.h>
#include <AutoPtr.h>
#include <epicsTimeHelper.h>
// Local

#include "ProcessVariable.h"
#include "ProcessVariableListener.h"

class PVTestPVListener : public ProcessVariableListener
{
public:
    int  num;
    bool connected;
    size_t values;
    
    PVTestPVListener(int num) : num(num), connected(false), values(0)
    {}
    
    void pvConnected(Guard &guard, ProcessVariable &pv,
                     const epicsTime &when)
    {
        printf("ProcessVariableListener %d: '%s' connected!\n",
               num, pv.getName().c_str());
        connected = true;
    }
    
    void pvDisconnected(Guard &guard, ProcessVariable &pv,
                        const epicsTime &when)
    {
        printf("ProcessVariableListener %d: PV '%s' disconnected!\n",
               num, pv.getName().c_str());
        connected = false;
    }
    
    void pvValue(class Guard &guard, class ProcessVariable &pv,
                 const RawValue::Data *data)
    {
        stdString tim, val;
        epicsTime2string(RawValue::getTime(data), tim);
        RawValue::getValueString(val,
                                 pv.getDbrType(guard),
                                 pv.getDbrCount(guard), data);
        printf("ProcessVariableListener %d: PV '%s' = %s %s\n",
               num, pv.getName().c_str(), tim.c_str(), val.c_str());
        ++values;
    }
};

TEST_CASE process_variable()
{
    AutoPtr<ProcessVariableContext> ctx(new ProcessVariableContext());
    AutoPtr<PVTestPVListener> pvl(new PVTestPVListener(1));
    AutoPtr<PVTestPVListener> pvl2(new PVTestPVListener(2));
    {
        AutoPtr<ProcessVariable> pv(new ProcessVariable(*ctx, "janet"));
        AutoPtr<ProcessVariable> pv2(new ProcessVariable(*ctx, "janet"));
        Guard pv_guard(*pv);
        Guard pv_guard2(*pv2);        
        pv->addListener(pv_guard, pvl);
        pv2->addListener(pv_guard2, pvl2);
        {
            Guard ctx_guard(*ctx);
            TEST(ctx->getRefs(ctx_guard) == 2);
        }
        TEST(pv->getName() == "janet"); 
        TEST(pv2->getName() == "janet"); 
        TEST(pv->getState(pv_guard) == ProcessVariable::INIT); 
        TEST(pv2->getState(pv_guard2) == ProcessVariable::INIT); 
        pv->start(pv_guard);
        pv2->start(pv_guard2);
        // PV ought to stay disconnected until the connection
        // gets sent out by the context in the following
        // flush handling loop.
        TEST(pv->getState(pv_guard) == ProcessVariable::DISCONNECTED); 
        TEST(pv2->getState(pv_guard2) == ProcessVariable::DISCONNECTED); 
        {
            GuardRelease release(pv_guard);
            GuardRelease release2(pv_guard2);
            size_t wait = 0;
            while (pvl->connected  == false  ||
                   pvl2->connected == false)
            {        
                epicsThreadSleep(0.1);
                {
                    Guard ctx_guard(*ctx);                    
                    if (ctx->isFlushRequested(ctx_guard))
                        ctx->flush(ctx_guard);
                }
                ++wait;
                if (wait > 100)
                    break;
            }
        }
        // Unclear if future releases might automatically connect
        // without flush.
        TEST(pv->getState(pv_guard) == ProcessVariable::CONNECTED);
        TEST(pv2->getState(pv_guard2) == ProcessVariable::CONNECTED);
        
        // 'get'
        puts("       Getting 1....");
        for (int i=0; i<3; ++i)
        {
            pvl->values = 0;
            pv->getValue(pv_guard);
            {   // The PV cannot deliver the result of the 'get'
                // while it's locked, so unlock:
                GuardRelease release(pv_guard);
                size_t wait = 0;
                while (pvl->values < 1)
                {        
                    epicsThreadSleep(0.1);
                    {
                        Guard ctx_guard(*ctx);                    
                        if (ctx->isFlushRequested(ctx_guard))
                            ctx->flush(ctx_guard);
                    }
                    ++wait;
                    if (wait > 10)
                        break;
                }
            }
        }
        epicsThreadSleep(1.0);

        // 'monitor'
        puts("       Monitoring 1, getting 2....");
        pvl->values = 0;
        pv->subscribe(pv_guard);
        pvl2->values = 0;
        pv2->getValue(pv_guard2);
        {
            // Unlock the PV so that it can deliver monitors:
            GuardRelease release(pv_guard);
            GuardRelease release2(pv_guard2);
            size_t wait = 0;
            while (pvl->values < 4)
            {        
                epicsThreadSleep(0.1);
                {
                    Guard ctx_guard(*ctx);                    
                    if (ctx->isFlushRequested(ctx_guard))
                        ctx->flush(ctx_guard);
                }
                ++wait;
                if (wait > 40)
                    break;
            }
        }
        
        TEST(pvl->values  >  0);
        TEST(pvl2->values == 1);
     
        pv->unsubscribe(pv_guard);
        pv->stop(pv_guard);
        pv2->stop(pv_guard2);
        pv->removeListener(pv_guard, pvl);
        pv2->removeListener(pv_guard2, pvl2);
    }
    {
        Guard ctx_guard(*ctx);
        TEST(ctx->getRefs(ctx_guard) == 0);
    }
    TEST_OK;
}
