class start {
  start(start&);
  start(start&&);
  start& operator=(start&);
  start& operator=(start&&);
  int method(int);
  int method(int&&);
  int&& method(int, int);
};
