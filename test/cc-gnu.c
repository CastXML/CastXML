#include <stdio.h>
#include <string.h>

#define DEFINE_FLT(x)                                                         \
  "#define __" #x "_DECIMAL_DIG__\n"                                          \
  "#define __" #x "_DENORM_MIN__\n"                                           \
  "#define __" #x "_DIG__\n"                                                  \
  "#define __" #x "_EPSILON__\n"                                              \
  "#define __" #x "_HAS_DENORM__\n"                                           \
  "#define __" #x "_HAS_INFINITY__\n"                                         \
  "#define __" #x "_HAS_QUIET_NAN__\n"                                        \
  "#define __" #x "_IS_IEC_60559__\n"                                         \
  "#define __" #x "_MANT_DIG__\n"                                             \
  "#define __" #x "_MAX_10_EXP__\n"                                           \
  "#define __" #x "_MAX_EXP__\n"                                              \
  "#define __" #x "_MAX__\n"                                                  \
  "#define __" #x "_MIN_10_EXP__\n"                                           \
  "#define __" #x "_MIN_EXP__\n"                                              \
  "#define __" #x "_MIN__\n"                                                  \
  "#define __" #x "_NORM_MAX__\n"                                             \
  ""

int main(int argc, char const* argv[])
{
  int cpp = 0;
  char const* std_date = 0;
  char const* ver_major = "1";
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
    } else if (strcmp(argv[i], "-fsized-deallocation") == 0) {
      fprintf(stdout, "#define __cpp_sized_deallocation 201309L\n");
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
  fprintf(stdout, DEFINE_FLT(BFLT16));
  fprintf(stdout, DEFINE_FLT(FLT32));
  fprintf(stdout, DEFINE_FLT(FLT32X));
  fprintf(stdout, DEFINE_FLT(FLT64));
  fprintf(stdout, DEFINE_FLT(FLT64X));
  fprintf(stdout, DEFINE_FLT(FLT128));
  fprintf(stdout,
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
