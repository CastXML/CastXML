int ordinary = 1;
constexpr int constant = 1;
struct Holder
{
  static constexpr int member = 1;
};
extern int redeclared;
#if __cplusplus > 201402L
inline int redeclared = 1;
extern int redeclared;
#endif
