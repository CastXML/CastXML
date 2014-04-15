#include <stdio.h>

int main(void)
{
  fprintf(stdout,
    "\n"
    "#define __cc_msvc__ 1\n"
    );
  return 0;
}
