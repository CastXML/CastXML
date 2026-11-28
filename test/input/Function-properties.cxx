inline int plain()
{
  return 1;
}
constexpr int constant()
{
  return 1;
}
struct S
{
  virtual void safe() noexcept;
  void yes() noexcept(sizeof(char) == 1);
  void maybe() noexcept(false);
  void ordinary();
};
