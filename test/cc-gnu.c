#include <stdio.h>
#include <string.h>

int main(int argc, const char* argv[])
{
  int i;
  for (i = 1; i < argc; ++i) {
    if (strncmp(argv[i], "--cc-define=", 12) == 0) {
      fprintf(stdout, "#define %s 1\n", argv[i]+12);
    }
  }
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
