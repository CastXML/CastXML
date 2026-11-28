namespace start {
struct Defaulted
{
  int n;
  Defaulted(Defaulted const&) = default;
};
struct UserProvided
{
  int n;
  UserProvided(UserProvided const&) {}
};
struct Forward;
static_assert(__is_trivially_copyable(Defaulted), "");
static_assert(!__is_trivially_copyable(UserProvided), "");
}
