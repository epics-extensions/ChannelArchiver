#include <math.h>
#include <epicsTimer.h>

#include "MsgLogger.cpp"
#include "GenericException.cpp"
#include "UnitTest.h"

static epicsTime  timer_test_start;

class OneShotTimer : public epicsTimerNotify
{
public:
    OneShotTimer(epicsTimerQueue &queue, double delay)
            : ok(true), timer(queue.createTimer()), delay(delay)
    {
    }

    ~OneShotTimer()
    {
        timer.destroy();
    }

    void start()
    {
        timer.start(*this, delay);
        LOG_MSG("OneShotTimer started by thread 0x%08lX\n",
                (unsigned long)epicsThreadGetIdSelf());
    }

    virtual expireStatus expire (const epicsTime & currentTime)
    {
        double time = epicsTime::getCurrent() - timer_test_start;
        LOG_MSG("OneShotTimer expires in thread 0x%08lX after %g secs\n",
                (unsigned long)epicsThreadGetIdSelf(), time);
        if (fabs(time - delay) > 0.2)
        {
            printf("Timer expired after %.1f instead of %.1f secs\n",
                   time, delay);
            ok = false;
        }
        return noRestart;
    }
    bool ok;
private:
    epicsTimer &timer;
    double     delay;
};

class PeriodTimer : public epicsTimerNotify
{
public:
    PeriodTimer(epicsTimerQueue &queue, double period)
            : ok(true),timer(queue.createTimer()), period(period)
    {
    }

    ~PeriodTimer()
    {
        timer.destroy();
    }

    void start()
    {
        timer.start(*this, period);
        period_start = epicsTime::getCurrent();
        LOG_MSG("PeriodTimer started by thread 0x%08lX\n",
                (unsigned long)epicsThreadGetIdSelf());
    }

    virtual expireStatus expire (const epicsTime & currentTime)
    {
        double time = epicsTime::getCurrent() - period_start;
        LOG_MSG("PeriodTimer expires in thread 0x%08lX after %g secs\n",
                (unsigned long)epicsThreadGetIdSelf(), time);
        if ( fabs(time - period) > 0.2 )
        {
            printf("Timer expired after %.1f instead of %.1f secs\n",
                   time, period);
            ok = false;
        }
        period_start = epicsTime::getCurrent();
        return expireStatus(restart, 1.5);
    }
    bool ok;
private:
    epicsTimer &timer;
    epicsTime  period_start;
    double     period;
};

// Passive Queue requires a handler, in here we ignore the reschedule...
// epicsTimerQueueNotify is defined with pure virtual protected destructor?!
class PassiveHandler : public epicsTimerQueueNotify
{
public:
    virtual void reschedule()
    {
    }
    virtual double quantum ()
    {
        return 0;
    }
    void destroy()
    {
        delete this;
    }
protected:
    virtual ~PassiveHandler()
    {
    }
};

TEST_CASE test_timer()
{
    epicsTimerQueueActive &act_queue = epicsTimerQueueActive::allocate(true);
    {
        timer_test_start = epicsTime::getCurrent();
        OneShotTimer once(act_queue, 4.0);
        PeriodTimer  period(act_queue, 1.5);
        once.start();
        period.start();
        epicsThreadSleep(10.0);
        double test_duration = epicsTime::getCurrent() - timer_test_start;
        TEST( fabs(test_duration - 10.0) < 0.1 );
        TEST( once.ok );
        TEST( period.ok );
    }
    act_queue.release();

    printf("\nThread Tests (single threaded)\n");
    printf("------------------------------------------\n");
    PassiveHandler *passive_handler = new PassiveHandler;
    epicsTimerQueuePassive &passive_queue =
        epicsTimerQueuePassive::create(*passive_handler);
    {
        timer_test_start = epicsTime::getCurrent();
        OneShotTimer once(passive_queue, 4.0);
        PeriodTimer  period(passive_queue, 1.5);
        once.start();
        period.start();
        double delay_to_next;
        epicsTime now = epicsTime::getCurrent();
        while (now - timer_test_start  < 10.0)
        {
            delay_to_next = passive_queue.process(now);
            if (delay_to_next > 1.0)
                delay_to_next = 1.0;
            epicsThreadSleep(delay_to_next);
            now = epicsTime::getCurrent();
        }
        TEST( once.ok );
        TEST( period.ok );
    }
    TEST_OK;
}

