#include <stdio.h>
#include <string.h>

int main(int argc, const char* argv[])
{
  int cpp = 0;
  const char* std_date = 0;
  const char* ver_major = "1";
  int i;
  for (i = 1; i < argc; ++i) {
    if (strncmp(argv[i], "--cc-define=", 12) == 0) {
      fprintf(stdout, "#define %s 1\n", argv[i] + 12);
    } else if (strncmp(argv[i], "--ver-major=", 12) == 0) {
      ver_major = argv[i] + 12;
    } else if (strncmp(argv[i], "-std=", 5) == 0) {
      std_date = argv[i] + 5;
    } else if (strcmp(argv[i], "-ansi") == 0) {
      fprintf(stdout, "#define __STRICT_ANSI__ 1\n");
    } else if (strcmp(argv[i], "-tgt-armv7") == 0) {
      fprintf(stdout, "#define __arm__ 1\n");
      fprintf(stdout, "#define __ARM_ARCH 7\n");
    } else if (strcmp(argv[i], "-tgt-arm64v8") == 0) {
      fprintf(stdout, "#define __aarch64__ 1\n");
      fprintf(stdout, "#define __ARM_ARCH 8\n");
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
          "#define __GNUC__ %s\n"
          "#define __has_include(x) x\n"
          "#define __has_include_next(x) x\n"
          "#define __GNUC_MINOR__ 1\n"
          "#define __bool __bool\n"
          "#define __builtin_vsx_foo __builtin_vsx_foo\n"
          "#define __pixel __pixel\n"
          "#define __vector __vector\n"
          "#define __has_last(x) x",
          ver_major);
  /* Test GCC builtin definitions for features Clang does not implement.  */
  fprintf(stdout,
          "#define __BFLT16_DECIMAL_DIG__\n"
          "#define __BFLT16_DENORM_MIN__\n"
          "#define __BFLT16_DIG__\n"
          "#define __BFLT16_DIG__\n"
          "#define __BFLT16_EPSILON__\n"
          "#define __BFLT16_HAS_DENORM__\n"
          "#define __BFLT16_HAS_INFINITY__\n"
          "#define __BFLT16_HAS_QUIET_NAN__\n"
          "#define __BFLT16_IS_IEC_60559__\n"
          "#define __BFLT16_MANT_DIG__\n"
          "#define __BFLT16_MAX_10_EXP__\n"
          "#define __BFLT16_MAX_EXP__\n"
          "#define __BFLT16_MAX__\n"
          "#define __BFLT16_MIN_10_EXP__\n"
          "#define __BFLT16_MIN_EXP__\n"
          "#define __BFLT16_MIN__\n"
          "#define __BFLT16_NORM_MAX__\n"
          "#define __SIZEOF_FLOAT80__\n"
          "#define __STDCPP_BFLOAT16_T__\n"
          "#define __STDCPP_FLOAT128_T__\n"
          "#define __STDCPP_FLOAT16_T__\n"
          "#define __STDCPP_FLOAT32_T__\n"
          "#define __STDCPP_FLOAT64_T__\n");
  fprintf(stderr,
          "#include <...> search starts here:\n"
          " /some/include\n"
          " " TEST_DIR "/cc-gnu-builtin\n"
          " /some/Frameworks\n"
          " /some/CustomFW (framework directory)\n");
  return 0;
}
