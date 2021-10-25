typedef struct foo_s
{
} foo;
void foo_close(foo*);
__attribute__((__malloc__, __malloc__(foo_close),
               __malloc__(foo_close, 1))) foo*
start(void);
