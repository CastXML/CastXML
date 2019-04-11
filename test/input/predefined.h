#ifndef __castxml_major__
#  error "__castxml_major__ not defined"
#endif
#ifndef __castxml_minor__
#  error "__castxml_minor__ not defined"
#endif
#ifndef __castxml_patch__
#  error "__castxml_patch__ not defined"
#endif
#ifndef __castxml__
#  error "__castxml__ not defined"
#endif
#ifndef __castxml_check
#  error "__castxml_check not defined"
#endif
#if __castxml__ < __castxml_check(0, 1, 0)
#  error "__castxml__ < __castxml_check(0, 1, 0)"
#endif
#if __castxml__ < __castxml_check(0, 0, 20000000)
#  error "__castxml__ < __castxml_check(0, 0, 20000000)"
#endif
#ifndef __castxml_clang_major__
#  error "__castxml_clang_major__ not defined"
#endif
#ifndef __castxml_clang_minor__
#  error "__castxml_clang_minor__ not defined"
#endif
#ifndef __castxml_clang_patchlevel__
#  error "__castxml_clang_patchlevel__ not defined"
#endif
