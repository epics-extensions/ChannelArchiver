
// Base
#include <epicsThread.h>
// Tools
#include <epicsTimeHelper.h>
#include <MsgLogger.h>
// Engine
#include "Engine.h"

Engine::Engine(const stdString &index_name, int port)
    : description("Archive Engine"),
      write_duration(0.0),
      write_count(0),
      process_delay_avg(0.0)
{
}

Engine::~Engine()
{
}

epicsMutex &Engine::getMutex()
{
    return mutex;
}
    
void Engine::setDescription(Guard &guard, const stdString &description)
{
    this->description = description;
}
    
void Engine::read_config(Guard &guard, const stdString &file_name)
{
    config.read(file_name.c_str(), this);
}

void Engine::addChannel(const stdString &group_name,
                        const stdString &channel_name,
                        double scan_period,
                        bool disabling, bool monitor)
{
    printf("Group '%s', Channel '%s': period %g, %s%s\n",
           group_name.c_str(), channel_name.c_str(), scan_period,
           (monitor ? "monitor" : "scan"),
           (disabling ? ", disabling" : ""));
}

static int test_runs = 5;

bool Engine::process()
{
    // Check if PV context requires a flush
    {
        Guard pv_ctxt_guard(pv_context);
        if (pv_context.isFlushRequested(pv_ctxt_guard))
            pv_context.flush(pv_ctxt_guard);
    }
    // scan, write or wait?
    epicsTime now = epicsTime::getCurrent();
#   define MAX_DELAY 5.0    
    double delay = MAX_DELAY;
    {   // Engine locked
        Guard engine_guard(*this);
        if (scan_list.isDueAtAll())
        {
            delay = scan_list.getDueTime() - now;
            if (delay <= 0.0)
            {
                scan_list.scan(now);
                process_delay_avg = 0.99*process_delay_avg;
                return true;
            }
        }
        double write_delay = next_write_time - now;            
        if (write_delay <= 0.0)
        {
            unsigned long count = writeArchive(engine_guard);
            epicsTime end = epicsTime::getCurrent();
            double duration = end - now;
            if (duration < 0.0)
                duration = 0.0;
            write_duration = 0.99*write_duration + 0.01*duration;
            write_count = (99*write_count + count)/100;
            next_write_time = roundTimeUp(end, config.getWritePeriod());
            process_delay_avg = 0.99*process_delay_avg;
            return true;
        }
        if (write_delay < delay)
            delay = write_delay;
    }
    // No scan, no write needed right now.
    process_delay_avg = 0.99*process_delay_avg + 0.01*delay;
    epicsThreadSleep(delay);

    if (--test_runs < 0)
        return false;
    return true;
}

unsigned long Engine::writeArchive(Guard &engine_guard)
{
    unsigned long count = 0;
    LOG_MSG("Engine: writing\n");
#if 0
    is_writing = true;
    try
    {
        IndexFile index(RTreeM);
        index.open(index_name, false);
        stdList<ArchiveChannel *>::iterator ch;
        for (ch = channels.begin(); ch != channels.end(); ++ch)
        {
            Guard guard((*ch)->mutex);
            count += (*ch)->write(guard, index);
        }
    }
    catch (GenericException &e)
    {
        LOG_MSG("Error while writing:\n%s\n", e.what());
    }
    try
    {
        DataFile::close_all();
    }
    catch (GenericException &e)
    {
        LOG_MSG("Error while closing data files:\n%s\n", e.what());
    }
    is_writing = false;
#endif
    //LOG_MSG("Engine: writing done.\n");
    return count;
}
