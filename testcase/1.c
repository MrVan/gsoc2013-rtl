int global;
static char local;

void hello(int arg)
{
  switch (arg) {
    case 1:
      printf("Inner-module call hello()\n");
      break;
    case 2:
      printf("Inter-module call hello()\n");
      break;
    default:
      return;
  }
}

#if defined (__arm__)
int rtems_arm(int arg)
{
  switch (arg) {
    case 24:	
      printf("R_ARM_JUMP24: arg = 24\n");
      break;
  }
  return 0;
}

int rtems_thumb(int arg)
{
  switch (arg) {
    case 19:
      printf("R_ARM_THM_JUMP19: arg = 19\n");
      break;
    case 24:
      printf("R_ARM_THM_JUMP24: arg = 24\n");
      break;
  }
  return 0;
}
#endif

/* This is the init entry, Because init() use "blx", it is not needed
 * to handle arm/thumb switch here
 */
int rtems(int argc, char **argv) 
{
  local = 1;
  printf("In rtems() local = %d\n", local); //inner-module data access
  hello(1); //inner-module call
  test(argc, argv);

#if defined (__arm__)
#ifdef THUMB_TEST
  __asm__ volatile (
      "push {r0}\r\n"
      "push {lr}\r\n"
      "mov r0, #19\r\n"
      "cmp r0, #19\r\n"
      "mov lr, pc\r\n"
      "add lr, #7\r\n"
      "beq rtems_thumb\r\n" /*THM_JUMP19*/
      "nop\r\n"
      "nop\r\n"
      "nop\r\n"
      "nop\r\n"
      "pop {lr}\r\n"
      "pop {r0}\r\n"
      );
  __asm__ volatile (
      "push {r0}\r\n"
      "push {lr}\r\n"
      "mov r0, #24\r\n"
      "mov lr, pc\r\n"
      "add lr, #7\r\n"
      "b.w rtems_thumb\r\n" /*THM_JUMP24*/
      "nop\r\n"
      "nop\r\n"
      "nop\r\n"
      "nop\r\n"
      "pop {lr}\r\n"
      "pop {r0}\r\n"
      );
#elif defined ARM_TEST
  /*arm instruction test*/
  __asm__ volatile (
      "push {r0}\r\n"
      "push {lr}\r\n"
      "mov r0, #24\r\n"
      "mov lr, pc\r\n"
      "add lr, #4\r\n"
      "b rtems_arm\r\n" /*JUMP24*/
      "nop\r\n"
      "nop\r\n"
      "nop\r\n"
      "nop\r\n"
      "pop {lr}\r\n"
      "pop {r0}\r\n"
      );
#endif

#else
  /* other archs */
#endif
  return 0;
}