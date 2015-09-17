#include <stdio.h>
#include <string.h>

int main(int argc, const char* argv[])
{
  int cpp = 0;
  int i;
  for (i = 1; i < argc; ++i) {
    if (strncmp(argv[i], "--cc-define=", 12) == 0) {
      fprintf(stdout, "#define %s 1\n", argv[i]+12);
    } else if (strstr(argv[i], ".cpp")) {
      cpp = 1;
    }
  }
  if (cpp) {
    fprintf(stdout,
      "#define __cplusplus 199711L\n"
      );
  }
  fprintf(stdout,
    "#define __GNUC__ 1\n"
    "#define __has_include(x) x\n"
    "#define __has_include_next(x) x\n"
    "#define __GNUC_MINOR__ 1\n"
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
