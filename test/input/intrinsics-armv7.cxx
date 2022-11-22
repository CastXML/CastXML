#if !defined(__ARM_ARCH) || __ARM_ARCH != 7
#  error "__ARM_ARCH is incorrectly not defined to 7"
#endif
#ifdef __ARM_FEATURE_DIRECTED_ROUNDING
#  error "__ARM_FEATURE_DIRECTED_ROUNDING incorrectly defined"
#endif
