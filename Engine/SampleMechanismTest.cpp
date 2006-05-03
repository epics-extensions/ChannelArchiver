
// Base
#include <epicsThread.h>
// Tools
#include <UnitTest.h>
#include <AutoPtr.h>
#include <epicsTimeHelper.h>
// Local
#include "SampleMechanism.h"
#include "DemoProcessVariableListener.h"

TEST_CASE test_sample_mechanism()
{
    EngineConfig config;
    ProcessVariableContext ctx;
    DemoProcessVariableListener listener;
    
    COMMENT("Testing the basic SampleMechanism...");
    SampleMechanism sample(config, ctx, "janet", 5.0, &listener);
    TEST(sample.getName() == "janet");
    COMMENT("Trying to connect...");
    {   
        Guard guard(__FILE__, __LINE__, sample);
        COMMENT(sample.getInfo(guard).c_str());
        sample.start(guard);
        TEST(sample.isRunning(guard));
    }
    size_t wait = 0;
    for (wait=1;  listener.connected==false && wait<10;  ++wait)
    {        
        epicsThreadSleep(0.1);
        {
            Guard ctx_guard(__FILE__, __LINE__, ctx);
            ctx.flush(ctx_guard);
        }
    }
    TEST(listener.connected == true);
    COMMENT("Disconnecting, hoping for 'disconnected' in the sample buffer.");
    {
        Guard guard(__FILE__, __LINE__, sample);
        COMMENT(sample.getInfo(guard).c_str());
        sample.stop(guard);
        COMMENT(sample.getInfo(guard).c_str());
        TEST(sample.getSampleCount(guard) == 1);
    }
    TEST(listener.connected == false);
    TEST_OK;
}


#if 0
#include "SampleMechanismGet.h"
#include "SampleMechanismMonitored.h"
#include "SampleMechanismMonitoredGet.h"

TEST_XX_CASE test_sample_get()
{
    puts("Note: This test takes 60 seconds.");
    ScanList scan_list;
    EngineConfig config;
    //config.setMaxRepeatCount(2);
    ProcessVariableContext ctx;
    {
        // Fred updates every ~20 seconds _when_polled_, otherwise 2Hz. 
        AutoPtr<SampleMechanism> sample(
            new SampleMechanismGet(config, ctx, scan_list, "fred", 5));
        TEST(sample->getName() == "fred");
        {
            Guard guard(__FILE__, __LINE__, *sample);
            sample->start(guard);
        }
        size_t wait = 0;
        while (true)
        {        
            epicsThreadSleep(0.1);
            {
                Guard ctx_guard(__FILE__, __LINE__, ctx);                    
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
            Guard guard(__FILE__, __LINE__, *sample);
            sample->stop(guard);
        }
        // Expect at least initial, initial host-stamped,
        // a repeat value, another, final 'off'
        {
            Guard guard(__FILE__, __LINE__, *sample);
            TEST(sample->getSampleCount(guard) > 4);
        }
    }
    TEST_OK;
}


TEST_XX_CASE test_sample_monitor()
{
    puts("Note: This test takes 2 seconds.");    
    EngineConfig config;
    ProcessVariableContext ctx;
    {
        AutoPtr<SampleMechanism> sample(
            new SampleMechanismMonitored(config, ctx, "janet", 1));
        TEST(sample->getName() == "janet");
        {
            Guard guard(__FILE__, __LINE__, *sample);
            sample->start(guard);
        }
        size_t wait = 0;
        while (true)
        {        
            epicsThreadSleep(0.1);
            {
                Guard ctx_guard(__FILE__, __LINE__, ctx);                    
                if (ctx.isFlushRequested(ctx_guard))
                    ctx.flush(ctx_guard);
            }
            ++wait;
            if (wait > 20)
                break;
        }
        {
            Guard guard(__FILE__, __LINE__, *sample);
            sample->stop(guard);
        }
        // Expect at least 10 samples
        {
            Guard guard(__FILE__, __LINE__, *sample);
            TEST(sample->getSampleCount(guard) > 10);
        }
    }
    TEST_OK;
}

TEST_XX_CASE test_sample_monitor_get()
{
    puts("Note: This test takes 60 seconds.");
    EngineConfig config;
    //config.setMaxRepeatCount(2);
    ProcessVariableContext ctx;
    {
        // Fred updates every ~20 seconds _when_polled_, otherwise 2Hz. 
        AutoPtr<SampleMechanism> sample(
            new SampleMechanismMonitoredGet(config, ctx, "fred", 5));
        TEST(sample->getName() == "fred");
        {
            Guard guard(__FILE__, __LINE__, *sample);
            sample->start(guard);
        }
        size_t wait = 0;
        while (true)
        {        
            epicsThreadSleep(0.1);
            {
                Guard ctx_guard(__FILE__, __LINE__, ctx);                    
                if (ctx.isFlushRequested(ctx_guard))
                    ctx.flush(ctx_guard);
            }
            ++wait;
            if (wait > 60*10)
                break;
        }
        {
            Guard guard(__FILE__, __LINE__, *sample);
            sample->stop(guard);
        }
        // Expect at least initial, initial host-stamped,
        // a repeat value, another, final 'off'
        {
            Guard guard(__FILE__, __LINE__, *sample);
            TEST(sample->getSampleCount(guard) > 4);
        }
    }
    TEST_OK;
}

#endif
