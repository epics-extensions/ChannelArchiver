// System
#include <stdio.h>
// Tools
#include <UnitTest.h>
// Storage
#include "IndexFile.h"
#include "SpreadsheetReader.h"
#include "DataFile.h"

TEST_CASE spreadsheet_test()
{
    stdString txt, stat;
    size_t i;
    IndexFile index;
    index.open("../DemoData/index");
    {
        SpreadsheetReader sheet(index);

        stdVector<stdString> names;
        names.push_back("fred");
        names.push_back("jane");

        bool found_any = sheet.find(names);
        TEST_MSG(found_any, "Found channels");
        TEST(sheet.getNum() == names.size());

        while (found_any)
        {   // Line: time ... 
            epicsTime2string(sheet.getTime(), txt);
            printf("%s", txt.c_str());
            // .. values ...
            for (i=0; i<sheet.getNum(); ++i)
            {
                const RawValue::Data *value = sheet.get(i);
                if (value)
                {
                    RawValue::getValueString(txt,
                                             sheet.getType(i), sheet.getCount(i),
                                             value, & sheet.getInfo(i));
                    RawValue::getStatus(value, stat);
                    printf(" | %20s %20s", txt.c_str(), stat.c_str());
                }
                else
                    printf(" | #N/A                                    ");
    
            }
            printf("\n");
            found_any = sheet.next();
        }
    }
    TEST(DataFile::clear_cache() == 0);
    TEST_OK;
}

