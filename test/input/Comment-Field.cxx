class start
{
  /** field comment */
  int field;
  /// bit field comment
  unsigned bit_field : 2;
  //! mutable field comment
  mutable int mutable_field;

public:
  start(start const&);
  start& operator=(start const&);
  start();
  ~start();
};
