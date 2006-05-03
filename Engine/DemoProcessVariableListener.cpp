// Local
#include "DemoProcessVariableListener.h"

DemoProcessVariableListener::DemoProcessVariableListener()
    : connected(false), values(0)
{}

DemoProcessVariableListener::~DemoProcessVariableListener()
{}

void DemoProcessVariableListener::pvConnected(ProcessVariable &pv,
                                              const epicsTime &when)
{
    printf("        DemoProcessVariableListener: '%s' connected.\n",
           pv.getName().c_str());
    connected = true;
}

void DemoProcessVariableListener::pvDisconnected(ProcessVariable &pv,
                                                 const epicsTime &when)
{
    printf("        DemoProcessVariableListener: '%s' disconnected.\n",
           pv.getName().c_str());
    connected = false;
}

void DemoProcessVariableListener::pvValue(class ProcessVariable &pv,
                                          const RawValue::Data *data)
{
    printf("        DemoProcessVariableListener: '%s' has value.\n",
               pv.getName().c_str());
    ++values;
}
