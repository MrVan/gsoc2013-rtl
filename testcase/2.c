extern int global;
extern void hello(int);

int test(int argc, char **argv)
{
  global = 1; //inter-module data access
  printf("In test() global = %d\n", global);
  hello(2); //inter-module call
}

#if defined (__mips__)
#if 1
void tst_pc16(void)
{
  printf("R_MIPS_PC16: 'b tst_pc16' \n");
  return;
}
#endif
#endif

int my_main(int argc, char **argv)
{
  exit(0);
}
