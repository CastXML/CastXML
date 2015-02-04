#include <stdio.h>

int main(void)
{
  fprintf(stdout,
    "#define __cc_gnu__ 1\n"
    "#define __has_include(x) x\n"
    "#define __has_include_next(x) x\n"
    "#define __cc_gnu_minor__ 1\n"
    "#define __has_last(x) x"
    );
  fprintf(stderr,
    "#include <...> search starts here:\n"
    " /some/include\n"
    " " TEST_DIR "/cc-gnu-builtin\n"
    " /some/Frameworks\n"
    " /some/CustomFW (framework directory)\n"
    );
  return 0;
}
