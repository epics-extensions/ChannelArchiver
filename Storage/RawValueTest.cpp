// Tools
#include <UnitTest.h>
// Storage
#include "RawValue.h"

// Not a full test because the formatted output
// isn't compared to the expected result;
// Only checking string length for consistency.
static bool fmt(double d)
{
    char buffer[50];
    size_t l;

    printf("\n");
    l = RawValue::formatDouble(d, RawValue::DEFAULT, 6, buffer, sizeof(buffer));
    printf("DEFAULT   : '%s' (%u)\n", buffer, (unsigned)l);
    if (strlen(buffer) != l) { FAIL("String length"); }

    l = RawValue::formatDouble(d, RawValue::DECIMAL, 6, buffer, sizeof(buffer));
    printf("DECIMAL   : '%s' (%u)\n", buffer, (unsigned)l);
    if (strlen(buffer) != l) { FAIL("String length"); }

    l = RawValue::formatDouble(d, RawValue::ENGINEERING, 6, buffer, sizeof(buffer));
    printf("ENGINEERING: '%s' (%u)\n", buffer, (unsigned)l);
    if (strlen(buffer) != l) { FAIL("String length"); }

    l = RawValue::formatDouble(d, RawValue::EXPONENTIAL, 6, buffer, sizeof(buffer));
    printf("EXPONENTIAL: '%s' (%u)\n", buffer, (unsigned)l);
    if (strlen(buffer) != l) { FAIL("String length"); }

    TEST_OK;
}

TEST_CASE RawValue_format()
{
    TEST(fmt(0.0));
    TEST(fmt(-0.321));
    TEST(fmt(1.0e-12));
    TEST(fmt(-1.0e-12));
    TEST(fmt(3.14e-7));
    TEST(fmt(3.14));
    TEST(fmt(3.14e+7));
    TEST(fmt(-3.14e+7));
    TEST(fmt(-0.123456789));
    TEST_OK;
}

