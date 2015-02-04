#include <stdio.h>

int main(void)
{
  fprintf(stdout,
    "#define __cc_gnu__ 1\n"
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
