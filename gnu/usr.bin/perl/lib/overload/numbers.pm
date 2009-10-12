# -*- buffer-read-only: t -*-
#
#   lib/overload/numbers.pm
#
#   Copyright (C) 2008 by Larry Wall and others
#
#   You may distribute under the terms of either the GNU General Public
#   License or the Artistic License, as specified in the README file.
#
# !!!!!!!   DO NOT EDIT THIS FILE   !!!!!!!
# This file is built by overload.pl
#

package overload::numbers;

our @names = qw#
    ()
    (${}
    (@{}
    (%{}
    (*{}
    (&{}
    (++
    (--
    (bool
    (0+
    (""
    (!
    (=
    (abs
    (neg
    (<>
    (int
    (<
    (<=
    (>
    (>=
    (==
    (!=
    (lt
    (le
    (gt
    (ge
    (eq
    (ne
    (nomethod
    (+
    (+=
    (-
    (-=
    (*
    (*=
    (/
    (/=
    (%
    (%=
    (**
    (**=
    (<<
    (<<=
    (>>
    (>>=
    (&
    (&=
    (|
    (|=
    (^
    (^=
    (<=>
    (cmp
    (~
    (atan2
    (cos
    (sin
    (exp
    (log
    (sqrt
    (x
    (x=
    (.
    (.=
    (~~
    DESTROY
#;

our @enums = qw#
    fallback
    to_sv
    to_av
    to_hv
    to_gv
    to_cv
    inc
    dec
    bool_
    numer
    string
    not
    copy
    abs
    neg
    iter
    int
    lt
    le
    gt
    ge
    eq
    ne
    slt
    sle
    sgt
    sge
    seq
    sne
    nomethod
    add
    add_ass
    subtr
    subtr_ass
    mult
    mult_ass
    div
    div_ass
    modulo
    modulo_ass
    pow
    pow_ass
    lshift
    lshift_ass
    rshift
    rshift_ass
    band
    band_ass
    bor
    bor_ass
    bxor
    bxor_ass
    ncmp
    scmp
    compl
    atan2
    cos
    sin
    exp
    log
    sqrt
    repeat
    repeat_ass
    concat
    concat_ass
    smart
    DESTROY
#;

{ my $i = 0; our %names = map { $_ => $i++ } @names }

{ my $i = 0; our %enums = map { $_ => $i++ } @enums }

