#include <stdio.h>
#include <string.h>

int main(int argc, const char* argv[])
{
  int i;
  for (i = 1; i < argc; ++i) {
    if (strncmp(argv[i], "--cc-define=", 12) == 0) {
      fprintf(stdout, "\n#define %s 1", argv[i]+12);
    }
  }
  fprintf(stdout,
    "\n"
    "#define __cplusplus 199711L\n"
    "#define _MSC_VER 1600\n"
    "#define __has_include(x) x\n"
    "#define __has_include_next(x) x\n"
    "#define _WIN32 1\n"
    "#define __has_last(x) x"
    );
  return 0;
}
