
// Base
#include <epicsThread.h>
// Tools
#include <UnitTest.h>
#include <AutoPtr.h>
#include <epicsTimeHelper.h>
// Local

#include "ProcessVariable.h"
#include "ProcessVariableListener.h"

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
        printf("ProcessVariableListener: PV '%s' disconnected!\n",
               pv.getName().c_str());
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
        printf("ProcessVariableListener: PV '%s' = %s %s\n",
               pv.getName().c_str(), tim.c_str(), val.c_str());
        ++values;
    }
};

TEST_CASE process_variable()
{
    AutoPtr<ProcessVariableContext> ctx(new ProcessVariableContext());
    AutoPtr<MyPVListener> pvl(new MyPVListener());
    {
        AutoPtr<ProcessVariable> pv(new ProcessVariable(*ctx, "janet"));
        Guard pv_guard(*pv);
        
        pv->addProcessVariableListener(pv_guard, pvl);
        {
            Guard ctx_guard(*ctx);
            TEST(ctx->getRefs(ctx_guard) == 1);
        }
        TEST(pv->getName() == "janet"); 
        TEST(pv->getState(pv_guard) == ProcessVariable::INIT); 
        pv->start(pv_guard);
        // PV ought to stay disconnected until the connection
        // gets sent out by the context in the following
        // flush handling loop.
        TEST(pv->getState(pv_guard) == ProcessVariable::DISCONNECTED); 
        {
            GuardRelease release(pv_guard);
            size_t wait = 0;
            while (pvl->connected == false)
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
        
        // 'get'
        puts("       Getting....");
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
        puts("       Monitoring....");
        pvl->values = 0;
        pv->subscribe(pv_guard);
        {   // Unlock the PV so that it can deliver monitors:
            GuardRelease release(pv_guard);
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

        pv->unsubscribe(pv_guard);
        pv->stop(pv_guard);
        pv->removeProcessVariableListener(pv_guard, pvl);
    }
    {
        Guard ctx_guard(*ctx);
        TEST(ctx->getRefs(ctx_guard) == 0);
    }
    TEST_OK;
}
