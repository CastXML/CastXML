extern int ordinary;
alignas(16) extern int small;
alignas(64) alignas(16) extern int large;
alignas(0) extern int zero;
extern int redeclared;
struct CacheLine
{
  alignas(64) char byte;
};
alignas(struct CacheLine) extern int redeclared;
extern int redeclared;
