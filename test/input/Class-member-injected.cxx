namespace start {
class A
{
  A method(A*, A**);
  A method(A&, A*&);
  A method(A&&, A*&&);
};
A function(A*, A**);
A function(A&, A*&);
A function(A&&, A*&&);
}
