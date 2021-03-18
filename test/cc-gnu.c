#include <stdio.h>
#include <string.h>

int main(int argc, const char* argv[])
{
  int cpp = 0;
  const char* std_date = 0;
  int i;
  for (i = 1; i < argc; ++i) {
    if (strncmp(argv[i], "--cc-define=", 12) == 0) {
      fprintf(stdout, "#define %s 1\n", argv[i] + 12);
    } else if (strncmp(argv[i], "-std=", 5) == 0) {
      std_date = argv[i] + 5;
    } else if (strcmp(argv[i], "-ansi") == 0) {
      fprintf(stdout, "#define __STRICT_ANSI__ 1\n");
    } else if (strstr(argv[i], ".cpp")) {
      cpp = 1;
    }
  }
  if (cpp) {
    fprintf(stdout, "#define __cplusplus %s\n",
            std_date ? std_date : "199711L");
  } else if (std_date) {
    fprintf(stdout, "#define __STDC_VERSION__ %s\n", std_date);
  }
  fprintf(stdout,
#ifdef _WIN32
          "#define _WIN32 1\n"
          "#define __MINGW32__ 1\n"
#endif
          "#define __GNUC__ 1\n"
          "#define __has_include(x) x\n"
          "#define __has_include_next(x) x\n"
          "#define __GNUC_MINOR__ 1\n"
          "#define __bool __bool\n"
          "#define __builtin_vsx_foo __builtin_vsx_foo\n"
          "#define __pixel __pixel\n"
          "#define __vector __vector\n"
          "#define __has_last(x) x");
  fprintf(stderr,
          "#include <...> search starts here:\n"
          " /some/include\n"
          " " TEST_DIR "/cc-gnu-builtin\n"
          " /some/Frameworks\n"
          " /some/CustomFW (framework directory)\n");
  return 0;
}
