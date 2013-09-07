#include "i.h"

int global;
static char local;

#if defined (__bfin__)
void bfin_test(void)
{
  printf("PCREL12_JUMP_S: JUMP.S _bfin_test\n");
  printf("Do not have a good idea about how to"
         "return from here, thus just halt here\n");
  while(1);
}
#endif

void hello(int arg)
{
  switch (arg) {
    case 1:
      printf("Inner-module call hello()\n");
      break;
    case 2:
      printf("Inter-module call hello()\n");
      break;
#if defined PPC
    case 24:
      printf("PPC ADDR24 'bla hello' call\n");
      break;
    case 14:
      printf("PPC REL14 'beq cr7, hello' jump\n");
      break;
    case 15:
      printf("PPC ADDR14 'beqa cr7, hello' jump\n");
      break;
#elif defined (__sparc__)
    case 22:
      printf("SPARC WDISP22 'b hello'\n");
      break;
    case 13:
      printf("SPARC 13 'mov hello, %%l4'\n");
      break;
#elif defined (__moxie__)
    case 10:
      printf("Just test 'beq hello, PCREL10', so just halt here\n");
      while(1);
      break;
#elif defined (__m32r__)
    case 18:
      printf("beq r0, r4, hello, 18_PCREL_RELA pass\n");
      break;
#else

#endif
    default:
#if defined (__m68k__)
      printf("M68K PC16 pass\n");
#else
      printf("no arg in hello\n");
#endif
      return;
  }
}

#if defined (__arm__)
int rtems_arm(int arg);
int rtems_arm(int arg)
{
  switch (arg) {
    case 24:
      printf("R_ARM_JUMP24: arg = 24\n");
      break;
  }
  return 0;
}

int rtems_thumb(int arg);
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

#elif defined PPC
  __asm__ volatile (
      "stwu 3, -8(1)\r\n"
      "li 3, 24\r\n"
      "bla hello\r\n" /*ADDR24*/
      "nop\r\n"
      "nop\r\n"
      "lwz 3, 8(1)\r\n"
      "addi 1, 1, 8\r\n"
      );

  __asm__ volatile (
      "stwu 3, -4(1)\r\n"
      "li 3, 14\r\n"
      "cmpwi cr7, 3, 14\r\n"
      "bl 1f\r\n"
      "1: mflr 6\r\n"
      "addi 6, 6, 20\r\n"
      "mtlr 6\r\n"
      "beq cr7, hello\r\n" /*REL14*/
      "nop\r\n"
      "nop\r\n"
      "nop\r\n"
      "lwz 3, 4(1)\r\n"
      "addi 1, 1, 4\r\n"
      );
#elif defined (__mips__)
  __asm__ volatile (
      "addiu $29, $29, -8\n\t"
      "sw $24,4($29)\n\t"
      "sw $25,0($29)\n\t"
      "la $24, 1f\n\t"
      "lw $24, 0($24)\n\t"
      "li $25, 32\t\n"
      "sw $25, 0($24)\n\t"
      "j 3f\n\t"
      "1:\n\t"
      ".word global\n\t"
      "3:\n\t"
      "lw $24,4($29)\n\t"
      "lw $25,0($29)\n\t"
      "addiu $29, $29, 8\n\t"
      );

  if (global == 32)
    printf("R_MIPS_32: '.word global' pass \n");
#if 1
    printf("R_MIPS_32: '.word global' pass \n");
#endif

  __asm__ volatile (
     "la $31, 1f\n\t"
     "b tst_pc16\n\t" /* R_MIPS_PC16 */
     "j tst_pc16\n\t"
     "1:\n\t"
     "nop\n\t"
      );

#elif defined (__sparc__)
  __asm__ volatile (
     "sethi %%hi(1f), %%l5\n\t"
     "ld [%%l5 + %%lo(1f)], %%l4\n\t"
     "mov 32, %%l5\n\t"
     "st %%l5, [%%l4]\n\t"
     "b 2f\n\t"
     "nop\n\t"
     "nop\n\t"
     "1:\n\t"
     ".word global\n\t" /* R_SPARC_32 */
     "2:\n\t"
     "nop\n\t"
     "nop\n\t" : : : "l4", "l5");

  if (global == 32)
    printf("R_SPARC_32: '.word global' pass\n");

  __asm__ volatile (
     "mov %%o0, %%l4\n\t"
     "mov 22, %%o0\n\t"
     "sethi %%hi(3f), %%o7\n\t"
     "or %%o7, %%lo(3f), %%o7\n\t"
     "b hello\n\t" /* R_SPARC_WDISP22 */
     "nop\n\t"
     "3:\t\n"
     "nop\n\t"
     "mov %%l4, %%o0\n\t"
     "nop\n\t" : : : "o0", "o7"
      );

  /* R_SPARC_13: Overflow are not checked, Thus
   * should use assemble language to handle this
   * to avoid overflow.
   */
  __asm__ volatile (
     "mov hello, %%l4\n\t"
     "and %%l4, 0x3ff, %%l4\n\t"
     "sethi %%hi(hello), %%l5\n\t"
     "or %%l4, %%l5, %%l4\n\t"
     "mov %%o0, %%l5\n\t"
     "mov 13, %%o0\n\t"
     "call hello\n\t"
     "nop\n\t"
     "mov %%l5, %%o0\n\t"
     "nop\n\t": : :
      );

#elif defined (__bfin__)
  __asm__ volatile (
      "JUMP.S _bfin_test\n\t"
      "1:\n\t"
      "nop\n\t"
      );
#elif defined (__lm32__)
  __asm__ volatile (
      "addi sp, sp, -16\n\t"
      "sw (sp+8), r1\n\t"
      "sw (sp+4), r0\n\t"
      "mvhi r1, 1f\n\t"
      "ori r1, r1, 1f\n\t"
      "lw r1, (r1+0)\n\t"
      "mvi r0, 22\n\t"
      "sw (r1+0), r0\n\t"
      "bi 2f\n\t"
      "1:\n\t"
      ".word global\n\t"
      "2:\n\t"
      "nop\n\t"
      "lw r1, (sp+8)\n\t"
      "lw r0, (sp+4)\n\t"
      "addi sp, sp, 16\n\t" : : : "r0", "r1"
      );

  if (global == 22)
    printf("R_LM32_32 pass\n");

#elif defined (__moxie__)
  __asm__ volatile (
      "ldi.l $r0, 10\n\t"
      "ldi.l $r1, 10\n\t"
      "cmp $r0, $r1\n\t"
      "beq hello\n\t");

#elif defined (__m32r__)
#if 1
  __asm__ volatile (
      "push r0\n\t"
      "push r4\n\t"
      "push r14\n\t"
      "ld24 r14, 2f\n\t"
      "ldi r0, #18\n\t"
      "ldi r4, #18\n\t"
      "beq r0, r4, hello\n\t"
      "1:\n\t"
      ".word global\n\t"
      "2:\n\t"
      "ld24 r0, 1b\n\t"
      "ld r4, @r0\n\t"
      "ldi r0, #22\n\t"
      "st r0, @r4\n\t"
      "pop r14\n\t"
      "pop r4\n\t"
      "pop r0\n\t"
      );
  if (global == 22)
    printf("R_M32R_32_RELA, .word global pass\n");

#endif

#elif defined (__m68k__)
  __asm__ volatile (
      ".align 2\n\t"
      "subal #4, %%a7\n\t"
      "lea 1f, %%a0\n\t"
      "movel %%a0, %%a7@(0)\n\t"
      "bra hello\n\t" /* R_68K_PC16 */
      "1:\n\t"
      "nop\n\t" : : :"a0", "a1"
      );
#else
  /* other archs */
#endif
  return 0;
}
