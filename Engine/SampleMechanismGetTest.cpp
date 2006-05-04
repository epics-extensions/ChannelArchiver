
// Base
#include <epicsThread.h>
// Tools
#include <UnitTest.h>
#include <AutoPtr.h>
#include <epicsTimeHelper.h>
// Local
#include "SampleMechanismGet.h"

TEST_CASE test_sample_mechanism_get()
{
    COMMENT("Note: This test takes 60 seconds.");
    ScanList scan_list;
    WritableEngineConfig config;
    config.setMaxRepeatCount(1);
    
    ProcessVariableContext ctx;
    SampleMechanismGet sample(config, ctx, scan_list, "fred", 5.0);
    TEST(sample.getName() == "fred");
    COMMENT("Trying to connect...");
    {   
        Guard guard(__FILE__, __LINE__, sample);
        COMMENT(sample.getInfo(guard).c_str());
        sample.start(guard);
        TEST(sample.isRunning(guard));
    }
    size_t wait = 0;
    while (wait < 10  &&  sample.getPVState() != ProcessVariable::CONNECTED)
    {        
        epicsThreadSleep(0.1);
        {
            Guard ctx_guard(__FILE__, __LINE__, ctx);
            ctx.flush(ctx_guard);
        }
        ++wait;
    }
    TEST(sample.getPVState() == ProcessVariable::CONNECTED);

    COMMENT("Waiting for 3 samples...");
    while (wait < 1200)
    {        
        epicsTime now = epicsTime::getCurrent();
        if (scan_list.getDueTime() < now)
        {
            COMMENT("Scan...");
            scan_list.scan(now);
        }
        epicsThreadSleep(0.1);
        {
            Guard ctx_guard(__FILE__, __LINE__, ctx);
            ctx.flush(ctx_guard);
        }
        {
            Guard guard(__FILE__, __LINE__, sample);
            if (sample.getSampleCount(guard) >= 3)
                break;
        }
        ++wait;
    }
    COMMENT("Disconnecting.");
    size_t values, after_disconnect;
    {
        Guard guard(__FILE__, __LINE__, sample);
        COMMENT(sample.getInfo(guard).c_str());
        values = sample.getSampleCount(guard);
        sample.stop(guard);
        COMMENT(sample.getInfo(guard).c_str());
        after_disconnect = sample.getSampleCount(guard);
    }
    TEST(values >= 3);
    int added_samples = after_disconnect - values;
    COMMENT("Check if 'disconnected' and 'off' were added");
    TEST(added_samples == 2);
    
    TEST(sample.getPVState() == ProcessVariable::INIT);
    TEST_OK;
}

