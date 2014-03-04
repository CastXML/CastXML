class base_public {};
class base_private {};
class base_protected {};
class start:
  public base_public,
  private base_private,
  virtual protected base_protected
{
};
