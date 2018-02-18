template <typename F>
F start(F f)
{
  return f;
}

void instantiate_start_with_lambda()
{
  start([]() {});
}
