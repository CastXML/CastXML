namespace start {

// Struct
struct Foo1;

Foo1* s1;
struct Foo1* s2; // elaborated
const Foo1* s3;
const struct Foo1* s4; // elaborated
typedef Foo1* s5;
typedef struct Foo1* s6; // elaborated

// Enum
enum Foo2
{
};

Foo2* e1;
enum Foo2* e2; // elaborated
const Foo2* e3;
const enum Foo2* e4; // elaborated
typedef Foo2* e5;
typedef enum Foo2* e6; // elaborated

// Union
union Foo3
{
};

Foo3* u1;
union Foo3* u2; // elaborated
const Foo3* u3;
const union Foo3* u4; // elaborated
typedef Foo3* u5;
typedef union Foo3* u6; // elaborated

// Class
class Foo4
{
};

Foo4* c1;
class Foo4* c2; // elaborated
const Foo3* c3;
const class Foo4* c4; // elaborated
typedef Foo4* c5;
typedef class Foo4* c6; // elaborated

// Function arguments
void func1(Foo1* a1, struct Foo1* a2);
void func2(Foo2* a3, enum Foo2* a4);
void func3(Foo3* a5, union Foo3* a6);
void func4(Foo4* a7, class Foo4* a8);
}
