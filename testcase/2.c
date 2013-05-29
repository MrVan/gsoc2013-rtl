extern int global;
extern void hello(int);

int test(int argc, char **argv)
{
  global = 1; //inter-module data access
  printf("In test() global = %d\n", global);
  hello(2); //inter-module call
}

int my_main(int argc, char **argv)
{
  exit(0);
}
