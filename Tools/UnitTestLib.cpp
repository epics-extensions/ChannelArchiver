
// System
#include <stdlib.h>
// Tools
#include "GenericException.h"

bool test_filediff(const char *filename1, const char *filename2)
{
    char diff_cmd[200];
    int len = snprintf(diff_cmd, sizeof(diff_cmd),
                       "diff '%s' '%s'", filename1, filename2);
    if (len < 0  ||  len >= sizeof(diff_cmd))
        throw GenericException(__FILE__, __LINE__,
                               "snprintf returned len %d", len);
    int result = system(diff_cmd);
    return result == 0;
}


