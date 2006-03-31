
// Base
#include <epicsThread.h>
// Tools
#include <UnitTest.h>
#include <AutoPtr.h>
#include <epicsTimeHelper.h>
// Local

#include "SampleMechanismMonitored.h"
#include "SampleMechanismGet.h"

TEST_CASE test_sample_get()
{
    ScanList scan_list;
    EngineConfig config;
    //config.setMaxRepeatCount(2);
    ProcessVariableContext ctx;
    //AutoPtr<MyPVListener> pvl(new MyPVListener());
    {
        // Fred updates every ~20 seconds _when_polled_, otherwise 2Hz. 
        AutoPtr<SampleMechanism> sample(
            new SampleMechanismGet(config, ctx, scan_list, "fred", 5));
        TEST(sample->getName() == "fred");
        {
            Guard guard(*sample);
            sample->start(guard);
        }
        size_t wait = 0;
        while (true)
        {        
            epicsThreadSleep(0.1);
            {
                Guard ctx_guard(ctx);                    
                if (ctx.isFlushRequested(ctx_guard))
                    ctx.flush(ctx_guard);
            }
            epicsTime now = epicsTime::getCurrent();
            if (scan_list.getDueTime() < now)
                scan_list.scan(now);
            ++wait;
            if (wait > 60*10)
                break;
        }
        {
            Guard guard(*sample);
            sample->stop(guard);
        }
    }
    TEST_OK;
}


TEST_CASE test_sample_monitor()
{
    EngineConfig config;
    ProcessVariableContext ctx;
    //AutoPtr<MyPVListener> pvl(new MyPVListener());
    {
        AutoPtr<SampleMechanism> sample(
            new SampleMechanismMonitored(config, ctx, "janet", 1));
        TEST(sample->getName() == "janet");
        {
            Guard guard(*sample);
            sample->start(guard);
        }
        size_t wait = 0;
        while (true)
        {        
            epicsThreadSleep(0.1);
            {
                Guard ctx_guard(ctx);                    
                if (ctx.isFlushRequested(ctx_guard))
                    ctx.flush(ctx_guard);
            }
            ++wait;
            if (wait > 20)
                break;
        }
        {
            Guard guard(*sample);
            sample->stop(guard);
        }
    }
    TEST_OK;
}

