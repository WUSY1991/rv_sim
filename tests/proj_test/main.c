#include "MyPrintf.h"
#include "test_float.h"
#include "testcase/ackermann.c"
void main()
{
    // printfsend((uint8_t *)"Hello World!\r\n", 14);
    MyPrintf("Hello World!\r\n");
    ackermann_test();
    test_float();
    // test_float1();
    while(1);
}





