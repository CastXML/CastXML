__inline int start(int n, ...)
{
  return start(n + __builtin_va_arg_pack_len(), __builtin_va_arg_pack());
}
