namespace start {
void f(long);
void f(int) = delete;
struct Blocked
{
  int& ref;
  void method() = delete;
};
}
