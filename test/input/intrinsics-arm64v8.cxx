#if !defined(__ARM_ARCH) || __ARM_ARCH != 8
#  error "__ARM_ARCH is incorrectly not defined to 8"
#endif
#ifndef __ARM_FEATURE_DIRECTED_ROUNDING
#  error "__ARM_FEATURE_DIRECTED_ROUNDING incorrectly not defined"
#endif
