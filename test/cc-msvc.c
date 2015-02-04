#include <stdio.h>

int main(void)
{
  fprintf(stdout,
    "\n"
    "#define __cc_msvc__ 1\n"
    "#define __has_include(x) x\n"
    "#define __has_include_next(x) x\n"
    "#define __cc_msvc_minor__ 1\n"
    "#define __has_last(x) x"
    );
  return 0;
}
