#include "MyPrintf.h"
#include "test_float.h"
void main()
{
    // printfsend((uint8_t *)"Hello World!\r\n", 14);
    MyPrintf("Hello World!\r\n");
    test_float();
    // test_float1();
    while(1);
}





