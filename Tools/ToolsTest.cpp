#ifdef TEST_CA
#include<cadef.h>

typedef enum
{
    not_connected,
    subscribed,
    got_data
} CA_State;
    
static CA_State state = not_connected;
static evid event_id;
static size_t num_monitors = 0;

static void event_cb(struct event_handler_args args)
{
    state = got_data;
    ++num_monitors;
    if (args.type == DBF_DOUBLE)
    {
        LOG_MSG("PV %s = %g in thread 0x%08X\n",
                (const char *)args.usr, *((double *)args.dbr),
                epicsThreadGetIdSelf());

    }
    else
    {
        LOG_MSG("PV %s type %ld in thread 0x%08X\n",
                (const char *)args.usr, args.type,
                epicsThreadGetIdSelf());
    }
}

static void connection_cb(struct connection_handler_args args)
{
    TEST(args.op == CA_OP_CONN_UP);

    LOG_MSG("PV %s connection_cb %s in thread 0x%08X\n",
            (const char *)ca_puser(args.chid),
            (args.op == CA_OP_CONN_UP ? "UP" : "DOWN"),
            epicsThreadGetIdSelf());
    
    if (args.op == CA_OP_CONN_UP &&
        state < subscribed)
    {
        ca_add_masked_array_event(ca_field_type(args.chid),
                                  ca_element_count(args.chid),
                                  args.chid,
                                  event_cb, ca_puser(args.chid),
                                  0.0, 0.0, 0.0, &event_id, 
                                  DBE_VALUE | DBE_ALARM | DBE_LOG);
        state = subscribed;
    }
}

void test_ca()
{
    printf("\nChannelAccess Tests (requires 'janet' from excas)\n");
    printf("------------------------------------------\n");
    SEVCHK(ca_context_create(ca_enable_preemptive_callback),
           "ca_context_create");

    LOG_MSG("Connecting from thread 0x%08X\n",
            epicsThreadGetIdSelf());

    chid chid;
    SEVCHK(ca_create_channel("janet",
                             connection_cb, (void *) "janet",
                             CA_PRIORITY_ARCHIVE,
                             &chid),
           "ca_create_channel");
    SEVCHK(ca_flush_io(), "ca_pend_io");
    epicsThreadSleep(5.0);
    
    LOG_MSG("Cleanup from thread 0x%08X\n",
            epicsThreadGetIdSelf());
    ca_clear_channel(chid);
    ca_context_destroy();

    TEST(state == got_data);
    printf("Received %zd monitored values\n",
           num_monitors);
    TEST(abs(num_monitors - 50) < 10);
}

#endif


#ifdef TEST_BITSET

#include "Bitset.h"

void test_bitset()
{
    printf("\nBitset  Tests\n");
    printf("------------------------------------------\n");
    BitSet s;
    TEST(strcmp(s.to_string().c_str(), "") == 0);
    s.grow(10);
    TEST(strcmp(s.to_string().c_str(), "0") == 0);
    s.set(1);
    s.set(2, true);
    s.set(0);
    TEST(strcmp(s.to_string().c_str(), "111") == 0);
    s.grow(40);
    TEST(strcmp(s.to_string().c_str(), "111") == 0);
    TEST(s.count() == 3);
    TEST(s.any() == true);
    s.clear(0);
    s.set(2, false);
    s.clear(1);
    TEST(s.count() == 0);
    TEST(s.any() == false);
    s.set(30);
    TEST(strcmp(s.to_string().c_str(), "1000000000000000000000000000000") == 0);
    TEST(s.test(30) == true);
    TEST(s.test(3) == false);
    TEST(s.capacity() == 64);
}

#endif

#ifdef TEST_FUX
#include "FUX.h"
void test_fux()
{
    printf("\nFUX-based XML Tests\n");
    printf("------------------------------------------\n");
    FUX fux;
    FUX::Element *xml_doc;

    try
    {
        xml_doc = fux.parse("does_not_exist.xml");
        xml_doc = (FUX::Element *)2;
    }
    catch (GenericException &e)
    {
        TEST("Caught Exception:");
        printf("%s", e.what());
    }
    TEST(xml_doc != (FUX::Element *)2);

    xml_doc = fux.parse("test.xml");
    TEST(xml_doc != 0);
    fux.DTD="../Engine/engineconfig.dtd";
    fux.dump(stdout);
    TEST(xml_doc->find("write_period") != 0);
    TEST(xml_doc->find("group") != 0);
    TEST(xml_doc->find("group")->name == "group");
    TEST(xml_doc->find("group")->value.empty());
    TEST(xml_doc->find("channel") == 0);

    FUX::Element *group = xml_doc->find("group");
    TEST(group->parent == xml_doc);
    TEST(group->find("name") != 0);
    TEST(group->find("name")->value == "Vac");
    TEST(group->find("channel") != 0);
    TEST(group->find("channel")->find("name") != 0);
    TEST(group->find("channel")->find("name")->value == "vac1");
    TEST(group->find("channel")->find("period")->value == "1");
    TEST(group->find("quark") == 0);

    try
    {
        xml_doc = fux.parse("damaged.xml");
        xml_doc = (FUX::Element *)2;
    }
    catch (GenericException &e)
    {
        TEST("Caught Exception:");
        printf("%s", e.what());
    }
    TEST(xml_doc != (FUX::Element *)2);
}
#endif

// -----------------------------------------------------------------
// -----------------------------------------------------------------
// -----------------------------------------------------------------

#include <epicsTime.h>

int main ()
{
    test_conversions();

    test_bin_io();

    test_auto_file_ptr();

    test_ascii_parser();

    test_exception();

#ifdef TEST_AUTOPTR
    test_autoptr();
#endif

#ifdef TEST_STRING
    test_string();
#endif

#ifdef TEST_BITSET
    test_bitset();
#endif

#ifdef TEST_AVL
    avl_test();
#endif

#ifdef TEST_FUX
    test_fux();
#endif
    
#ifdef TEST_TIME
    struct local_tm_nano_sec tm;
    tm.ansi_tm.tm_year = 2003 - 1900;
    tm.ansi_tm.tm_mon  = 4 - 1;
    tm.ansi_tm.tm_mday = 10;
    tm.ansi_tm.tm_hour = 11;
    tm.ansi_tm.tm_min  = 0;
    tm.ansi_tm.tm_sec  = 0;
    tm.ansi_tm.tm_isdst   = -1;
    tm.nSec = 0;
    epicsTime time = tm;
    time.show(10);
   
    // At Wed Apr  9 10:43:37 MDT 2003 (daylight saving on),
    //  Win32 adds 1hour...
    // Convert 03/18/1990 12:13:44.800000019L back and forth:
    tm.ansi_tm.tm_year = 1990 - 1900;
    tm.ansi_tm.tm_mon  = 3 - 1;
    tm.ansi_tm.tm_mday = 18;
    tm.ansi_tm.tm_hour = 12;
    tm.ansi_tm.tm_min  = 13;
    tm.ansi_tm.tm_sec  = 44;
    tm.ansi_tm.tm_isdst   = -1;
    tm.nSec = 800000019L;
    
    // to epicsTime
    time = tm;

    // back to tm
    tm = time;
    const char *dst;
    switch (tm.ansi_tm.tm_isdst)
    {
        case 0:
            dst = "standard";
            break;
        case 1:
            dst = "daylight saving";
            break;
        default:
            dst = "unknown";
    }
    printf("%02d/%02d/%04d %02d:%02d:%02d.%09ld (%s)\n",
           tm.ansi_tm.tm_mon + 1,
           tm.ansi_tm.tm_mday,
           tm.ansi_tm.tm_year + 1900,
           tm.ansi_tm.tm_hour,
           tm.ansi_tm.tm_min,
           tm.ansi_tm.tm_sec,
           tm.nSec,
           dst);    
    test_time();
#endif
#ifdef TEST_LOG
    test_log();
#endif
#ifdef TEST_THREADS
    test_threads();
#endif
#ifdef TEST_TIMER
    test_timer();
#endif
#ifdef TEST_CA
    test_ca();
#endif

    return 0;
}
