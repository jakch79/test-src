/* PR c/11420 */
/* { dg-do link } */
/* { dg-options "-O2 -fpic" } */

void (* volatile fn) (void);
static void foo (void)
{
}

int main (void)
{
  fn = foo;
  return 0;
}
