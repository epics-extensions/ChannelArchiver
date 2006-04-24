// Base
#include <cadef.h>
// Tools
#include <GenericException.h>
#include <MsgLogger.h>
// Local
#include "ProcessVariableContext.h"

static void caException(struct exception_handler_args args)
{
    const char *pName;
    
    if (args.chid)
        pName = ca_name(args.chid);
    else
        pName = "?";
    LOG_MSG("CA Exception %s - with request "
            "chan=%s op=%d type=%s count=%d:\n%s\n", 
            args.ctx, pName, (int)args.op, dbr_type_to_text(args.type),
            (int)args.count,
            ca_message(args.stat));
}

ProcessVariableContext::ProcessVariableContext()
    : is_running(false), ca_context(0), refs(0), flush_requested(false)
{
	LOG_MSG("Creating ChannelAccess Context.\n");
	if (ca_context_create(ca_enable_preemptive_callback) != ECA_NORMAL ||
        ca_add_exception_event(caException, 0) != ECA_NORMAL)
        throw GenericException(__FILE__, __LINE__,
                               "CA client initialization failed.");
    ca_context = ca_current_context();
}

ProcessVariableContext::~ProcessVariableContext()
{
	if (refs > 0)
	{
        LOG_MSG("ProcessVariableContext has %zu references "
                "left on destruction\n", refs);
        return;
	}
	LOG_MSG("Stopping ChannelAccess.\n");
    ca_context_destroy();
    ca_context = 0;
}

epicsMutex &ProcessVariableContext::getMutex()
{
    return mutex;
}

void ProcessVariableContext::start(Guard &guard)
{
    guard.check(__FILE__, __LINE__, mutex);
    is_running = true;
}

bool ProcessVariableContext::isRunning(Guard &guard)
{
    guard.check(__FILE__, __LINE__, mutex);
    return is_running;
}

void ProcessVariableContext::stop(Guard &guard)
{
    guard.check(__FILE__, __LINE__, mutex);
    is_running = false;
}

void ProcessVariableContext::attach(Guard &guard)
{
    guard.check(__FILE__, __LINE__, mutex);
    int result = ca_attach_context(ca_context);
    if (result == ECA_ISATTACHED)
    	throw GenericException(__FILE__, __LINE__,
        	"thread 0x%08lX (%s) tried to attach more than once\n",
                (unsigned long)epicsThreadGetIdSelf(),
                epicsThreadGetNameSelf());
	if (result != ECA_NORMAL)
    	throw GenericException(__FILE__, __LINE__,
        	"ca_attach_context failed for thread 0x%08lX (%s)\n",
                (unsigned long)epicsThreadGetIdSelf(),
                epicsThreadGetNameSelf());
}

bool ProcessVariableContext::isAttached(Guard &guard)
{
    guard.check(__FILE__, __LINE__, mutex);
    int result = ca_attach_context(ca_context);
    return result == ECA_ISATTACHED;
}

void ProcessVariableContext::incRef(Guard &guard)
{
    guard.check(__FILE__, __LINE__, mutex);
    LOG_ASSERT(isAttached(guard));
	++refs;
}
		
void ProcessVariableContext::decRef(Guard &guard)
{
    guard.check(__FILE__, __LINE__, mutex);
    LOG_ASSERT(isAttached(guard));
	if (refs <= 0)
	    throw GenericException(__FILE__, __LINE__,
	                           "RefCount goes negative");
	--refs;
}

size_t ProcessVariableContext::getRefs(Guard &guard)
{
    guard.check(__FILE__, __LINE__, mutex);
    LOG_ASSERT(isAttached(guard));
	return refs;
}

void ProcessVariableContext::requestFlush(Guard &guard)
{
    guard.check(__FILE__, __LINE__, mutex);
    LOG_ASSERT(isAttached(guard));
	flush_requested = true;
}
	
bool ProcessVariableContext::isFlushRequested(Guard &guard)
{
    guard.check(__FILE__, __LINE__, mutex);
    LOG_ASSERT(isAttached(guard));
	return flush_requested;
}

void ProcessVariableContext::flush(Guard &guard)
{
    guard.check(__FILE__, __LINE__, mutex);
    LOG_ASSERT(isAttached(guard));
    flush_requested = false;
    {
        GuardRelease release(guard);
	    ca_flush_io();
    }
}

