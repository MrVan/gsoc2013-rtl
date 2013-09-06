#include <stdlib.h>
#include <stdio.h>

void hello(int arg);

#if defined (__arm__)
int rtems_arm(int arg);
int rtems_thumb(int arg);
#endif

#if defined (__mips__)
void tst_pc16(void);
#endif

#if defined (__sparc__)
void tst_wdisp(void);
#endif

int rtems(int argc, char* argv[]);
int test(int argc, char* argv[]);
int my_main(int argc, char* argv[]);

