/* { dg-do compile { target powerpc*-*-* } } */
/* { dg-options "-maltivec -mabi=altivec -fno-inline" } */

#include <altivec.h>
#include <signal.h>

void 
sig_ill_handler (int sig)
{
    exit(0);
}

typedef union
{
  float         f[4];
  unsigned int  i[4];
  vector float  v;
} vec_float_t;

void 
check_vec_all_num ()
{
  vec_float_t a, b, c;

  a.i[0] = 0xfffa5a5a;
  a.f[1] = 1.0;
  a.f[2] = 1.0;
  a.f[3] = 1.0;

  b.f[0] = 1.0;
  b.f[1] = 1.0;
  b.f[2] = 1.0;
  b.f[3] = 1.0;

  c.i[0] = 0xfffa5a5a;
  c.i[1] = 0xfffa5a5a;
  c.i[2] = 0xfffa5a5a;
  c.i[3] = 0xfffa5a5a;

  if (vec_all_numeric (a.v))
    abort ();

  if (vec_all_nan (a.v))
    abort ();

  if (!vec_all_numeric (b.v))
    abort ();

  if (vec_all_nan (b.v))
    abort ();

  if (vec_all_numeric (c.v))
    abort ();

  if (!vec_all_nan (c.v))
    abort ();

}

void 
check_cmple()
{
  vector float a = {1.0, 2.0, 3.0, 4.0};
  vector float b = {1.0, 3.0, 2.0, 5.0};
  vector signed int aux;
  vector signed int le = {-1, -1, 0, -1};

  aux = vec_cmple (a, b);

  if (!vec_all_eq (aux, le))
    abort ();
}


int 
main()
{
  /* Exit on systems without altivec.  */
  signal (SIGILL, sig_ill_handler);
  asm volatile ("vor 0,0,0");
  signal (SIGILL, SIG_DFL);

  check_cmple ();
  check_vec_all_num ();
  exit (0);
}
