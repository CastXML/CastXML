extern int ordinary;
_Alignas(16) extern int small;
_Alignas(64) _Alignas(16) extern int large;
_Alignas(0) extern int zero;
extern int redeclared;
struct CacheLine
{
  _Alignas(64) char byte;
};
_Alignas(struct CacheLine) extern int redeclared;
extern int redeclared;
