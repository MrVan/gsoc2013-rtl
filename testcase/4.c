#include <stdio.h>
int rtems(int argc, char **argv)
{
  int a;
  a =ar_func_test();
  printf("a = 0x%x\n",a);
  return 0;
}
