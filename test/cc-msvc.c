#include <stdio.h>
#include <string.h>

int main(int argc, const char* argv[])
{
  int cpp = 0;
  const char* msc_ver = "1600";
  const char* msvc_lang = 0;
  int i;
  for (i = 1; i < argc; ++i) {
    if (strncmp(argv[i], "--cc-define=", 12) == 0) {
      fprintf(stdout, "\n#define %s 1", argv[i]+12);
    } else if (strncmp(argv[i], "-msc=", 5) == 0) {
      msc_ver = argv[i]+5;
    } else if (strncmp(argv[i], "-msvc_lang=", 8) == 0) {
      msvc_lang = argv[i]+11;
    } else if (strstr(argv[i], ".cpp")) {
      cpp = 1;
    }
  }
  fprintf(stdout,
    "\n"
    );
  if (cpp) {
    fprintf(stdout,
      "#define __cplusplus 199711L\n"
      );
  }
  fprintf(stdout,
    "#define _MSC_VER %s\n", msc_ver
    );
  if(msvc_lang) {
    fprintf(stdout,
      "#define _MSVC_LANG %s\n", msvc_lang
      );
  }
  fprintf(stdout,
    "#define __has_include(x) x\n"
    "#define __has_include_next(x) x\n"
    "#define _WIN32 1\n"
    "#define __has_last(x) x"
    );
  return 0;
}
