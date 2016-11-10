class base
{
protected:
  base();
  ~base();

private:
  base(base const&);
  base& operator=(base const&);
};
class start : public base
{
};
