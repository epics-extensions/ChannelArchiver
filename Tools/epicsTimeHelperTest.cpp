
#include "ToolsConfig.h"
#include "epicsTimeHelper.h"
#include "UnitTest.h"

TEST_CASE test_time()
{
    initEpicsTimeHelper();

    epicsTime t;
    TEST(isValidTime(t) == false);
    
    t = epicsTime::getCurrent();
    epicsTime start = t;
    TEST(isValidTime(t) == true);
    
    epicsTimeStamp stamp;
    memset(&stamp, 0, sizeof(epicsTimeStamp));
    t = stamp;
    TEST(isValidTime(t) == false);
    
    epicsTime now = epicsTime::getCurrent();
    stdString s_start, s_now;
    epicsTime2string(start, s_start);
    epicsTime2string(now, s_now);
    //printf("Start: %s\n", s_start.c_str());
    //printf("Now:   %s\n", s_now.c_str());
    TEST(start <= now);
    TEST(now > start);
    t = now;
    TEST(t == now);

    int year, month, day, hour, min, sec;
    unsigned long nano;
    epicsTime2vals(start, year, month, day, hour, min, sec, nano);
    char buf[50];
    sprintf(buf, "%02d/%02d/%04d %02d:%02d:%02d.%09lu",
            month, day, year, hour, min, sec, nano);
    puts(buf);
    TEST(strcmp(s_start.c_str(), buf) == 0);

    vals2epicsTime(year, month, day, hour, min, sec, nano, t);
    TEST(t == start);

    t += 60;
    TEST(t != start);
    TEST(t > start);
    TEST(t - start == 60.0);

    stdString txt;
    vals2epicsTime(1990, 3, 18, 12, 13, 44, 800000019L, now);
    epicsTime2string(now, txt); puts(txt.c_str());           TEST(txt == "03/18/1990 12:13:44.800000019");
    epicsTime2string(roundTimeDown(now, 0), txt);            TEST(txt == "03/18/1990 12:13:44.800000019");
    epicsTime2string(roundTimeDown(now, 0.5), txt);          TEST(txt == "03/18/1990 12:13:44.500000000");
    epicsTime2string(roundTimeDown(now, 1.0), txt);          TEST(txt == "03/18/1990 12:13:44.000000000");
    epicsTime2string(roundTimeDown(now, 10.0), txt);         TEST(txt == "03/18/1990 12:13:40.000000000");
    epicsTime2string(roundTimeDown(now, 30.0), txt);         TEST(txt == "03/18/1990 12:13:30.000000000");

    epicsTime2string(roundTimeDown(now, 50.0), txt);
    printf("Rounded by 50 secs: %s\n", txt.c_str());

    epicsTime2string(roundTimeDown(now, 60.0), txt);         TEST(txt == "03/18/1990 12:13:00.000000000");
    epicsTime2string(roundTimeDown(now, secsPerHour), txt);  TEST(txt == "03/18/1990 12:00:00.000000000");
    epicsTime2string(roundTimeDown(now, secsPerDay), txt);   TEST(txt == "03/18/1990 00:00:00.000000000");
    epicsTime2string(roundTimeDown(now, secsPerMonth), txt); TEST(txt == "03/01/1990 00:00:00.000000000");
    epicsTime2string(roundTimeDown(now, secsPerYear), txt);  TEST(txt == "01/01/1990 00:00:00.000000000");

    vals2epicsTime(1990, 3, 18, 12, 13, 44, 800000019L, now);
    epicsTime2string(roundTimeUp(now, 0), txt);            TEST(txt == "03/18/1990 12:13:44.800000019");
    epicsTime2string(roundTimeUp(now, 0.1), txt);          TEST(txt == "03/18/1990 12:13:44.900000000");    
    epicsTime2string(roundTimeUp(now, 0.5), txt);          TEST(txt == "03/18/1990 12:13:45.000000000");    
    epicsTime2string(roundTimeUp(now, 1.0), txt);          TEST(txt == "03/18/1990 12:13:45.000000000");
    epicsTime2string(roundTimeUp(now, 10.0), txt);         TEST(txt == "03/18/1990 12:13:50.000000000");
    epicsTime2string(roundTimeUp(now, 30.0), txt);         TEST(txt == "03/18/1990 12:14:00.000000000");
    
    epicsTime2string(roundTimeUp(now, 50.0), txt);
    printf("Rounded by 50 secs: %s\n", txt.c_str());
    
    epicsTime2string(roundTimeUp(now, 60.0), txt);         TEST(txt == "03/18/1990 12:14:00.000000000");
    epicsTime2string(roundTimeUp(now, secsPerHour), txt);  TEST(txt == "03/18/1990 13:00:00.000000000");
    
    epicsTime2string(roundTimeUp(now, secsPerDay), txt);   TEST(txt == "03/19/1990 00:00:00.000000000");
    epicsTime2string(roundTimeUp(now, secsPerMonth), txt); TEST(txt == "04/01/1990 00:00:00.000000000");
    epicsTime2string(roundTimeUp(now, secsPerYear), txt);  TEST(txt == "01/01/1991 00:00:00.000000000");
    
    vals2epicsTime(2000, 12, 31, 23, 59, 59, 999999999L, now);
    epicsTime2string(roundTimeDown(now, 0), txt);            TEST(txt == "12/31/2000 23:59:59.999999999");
    epicsTime2string(roundTimeDown(now, secsPerDay), txt);   TEST(txt == "12/31/2000 00:00:00.000000000");
    epicsTime2string(roundTimeDown(now, secsPerYear), txt);  TEST(txt == "01/01/2000 00:00:00.000000000");

    vals2epicsTime(1990, 3, 18, 23, 13, 44, 800000019L, now);
    epicsTime2string(roundTimeUp(now, 60*60*1), txt);        TEST(txt == "03/19/1990 00:00:00.000000000");
    epicsTime2string(roundTimeUp(now, 60*60*2), txt);        TEST(txt == "03/19/1990 01:00:00.000000000");

    vals2epicsTime(1990, 3, 18, 01, 13, 44, 800000019L, now);
    epicsTime2string(roundTimeUp(now, 60*60*1), txt);        TEST(txt == "03/18/1990 02:00:00.000000000");

    printf("Rounding up by 15 minutes:\n");
    vals2epicsTime(1990, 3, 18, 01, 13, 44, 800000019L, now);
    now = roundTimeUp(now, 900);    printf("%s\n", epicsTimeTxt(now, txt));
    now = roundTimeUp(now, 900);    printf("%s\n", epicsTimeTxt(now, txt));
    now = roundTimeUp(now, 900);    printf("%s\n", epicsTimeTxt(now, txt));
    now = roundTimeUp(now, 900);    printf("%s\n", epicsTimeTxt(now, txt));
    now = roundTimeUp(now, 900);    printf("%s\n", epicsTimeTxt(now, txt));
    now = roundTimeUp(now, 900);    printf("%s\n", epicsTimeTxt(now, txt));
    TEST_OK;
}
