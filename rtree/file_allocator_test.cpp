#include <stdlib.h>
#include <string.h>
#include "file_allocator.h"

#define TEST(call,test)              \
if ((call))                          \
    printf("OK: %s\n", test);        \
else                                 \
    printf("FAIL: %s\n", test)

int main()
{
    FILE *f = fopen("test/file_allocator.dat", "r+b");
    if (f == 0)
        f = fopen("test/file_allocator.dat", "w+b");
    
    file_allocator fa;
    long o1, o2, o3, o4, o5, o6, o7;
    puts("-----------------------------------------");
    TEST(fa.attach(f, 1000), "attach");
    fa.dump();
    puts("-----------------------------------------");
    TEST(o1=fa.allocate(1000), "allocate 1000");
    printf("Got offset %ld\n", o1);
    fa.dump();
    puts("-----------------------------------------");
    TEST(o2=fa.allocate(2000), "allocate 2000");
    printf("Got offset %ld\n", o2);
    fa.dump();
    puts("-----------------------------------------");
    TEST(o3=fa.allocate(3000), "allocate 3000");
    printf("Got offset %ld\n", o3);
    fa.dump();
    puts("-----------------------------------------");
    TEST(o4=fa.allocate(4000), "allocate 4000");
    printf("Got offset %ld\n", o4);
    fa.dump();
    puts("-----------------------------------------");
    TEST(o5=fa.allocate(5000), "allocate 5000");
    printf("Got offset %ld\n", o5);
    fa.dump();
    puts("-----------------------------------------");
    TEST(o6=fa.allocate(6000), "allocate 6000");
    printf("Got offset %ld\n", o6);
    fa.dump();
    puts("-----------------------------------------");
    TEST(o7=fa.allocate(7000), "allocate 7000");
    printf("Got offset %ld\n", o7);
    fa.dump();
    puts("-----------------------------------------");
    TEST(fa.free(o1), "free 1000");
    fa.dump();
    puts("-----------------------------------------");
    TEST(fa.free(o7), "free 7000");
    fa.dump();
    puts("-----------------------------------------");
    TEST(fa.free(o5), "free 5000");
    fa.dump();
    puts("-----------------------------------------");
    TEST(fa.free(o4), "free 4000");
    fa.dump();
    puts("-----------------------------------------");
    TEST(fa.free(o2), "free 2000");
    fa.dump();
    puts("-----------------------------------------");
    TEST(fa.free(o3), "free 3000");
    fa.dump();
    puts("-----------------------------------------");
    TEST(o1=fa.allocate(4000), "allocate 4000");
    printf("Got offset %ld\n", o1);
    fa.dump();
    puts("-----------------------------------------");
    TEST(o2=fa.allocate(500), "allocate 500");
    printf("Got offset %ld\n", o2);
    fa.dump();
    puts("-----------------------------------------");
    TEST(o3=fa.allocate(1500), "allocate 1500");
    printf("Got offset %ld\n", o3);
    fa.dump();
    puts("-----------------------------------------");
    fa.detach();
    fclose(f);

    f = fopen("test/file_allocator.dat", "r+b");
    TEST(fa.attach(f, 1000), "re-attach");
    fa.dump();


    puts("-- upping file size increment to 50000 --");
    file_allocator::file_size_increment = 50000;
    TEST(o1=fa.allocate(10000), "allocate 10000");
    printf("Got offset %ld\n", o1);
    fa.dump();
    puts("-----------------------------------------");
    TEST(o1=fa.allocate(10000), "allocate 10000");
    printf("Got offset %ld\n", o1);
    fa.dump();
    puts("-----------------------------------------");
    TEST(o1=fa.allocate(10000), "allocate 10000");
    printf("Got offset %ld\n", o1);
    fa.dump();
    puts("-----------------------------------------");
    TEST(o2=fa.allocate(5000), "allocate 5000");
    printf("Got offset %ld\n", o2);
    fa.dump();
    puts("-----------------------------------------");
    TEST(o3=fa.allocate(5000), "allocate 5000");
    printf("Got offset %ld\n", o3);
    fa.dump();
    puts("-----------------------------------------");

    
    puts("-----------------------------------------");
    puts("Performing some random tests");
    long o[10];
    memset(o, 0, sizeof(o));
    int i;
    for (i=0; i<500; ++i)
    {
        if (o[i%10] && rand()>RAND_MAX/2)
        {
            if (! fa.free(o[i%10]))
                printf("Free failed\n");
            o[i%10] = 0;
        }
        else
        {
            o[i%10] = fa.allocate(rand() & 0xFFFF);
            if (o[i%10] == 0)
                printf("Alloc failed\n");
        }
        fa.dump(0); // consistency check
    }
    puts("If you saw no messages, all should be fine");
    fa.detach();
    fclose(f);

    
    return 0;
}

