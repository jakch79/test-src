################################################################################
#
#            !!!!!   Do NOT edit this file directly!   !!!!!
#
#            Edit mktests.PL and/or parts/inc/exception instead.
#
#  This file was automatically generated from the definition files in the
#  parts/inc/ subdirectory by mktests.PL. To learn more about how all this
#  works, please read the F<HACKERS> file that came with this distribution.
#
################################################################################

BEGIN {
  if ($ENV{'PERL_CORE'}) {
    chdir 't' if -d 't';
    @INC = ('../lib', '../ext/Devel-PPPort/t') if -d '../lib' && -d '../ext';
    require Config; import Config;
    use vars '%Config';
    if (" $Config{'extensions'} " !~ m[ Devel/PPPort ]) {
      print "1..0 # Skip -- Perl configured without Devel::PPPort module\n";
      exit 0;
    }
  }
  else {
    unshift @INC, 't';
  }

  sub load {
    eval "use Test";
    require 'testutil.pl' if $@;
  }

  if (7) {
    load();
    plan(tests => 7);
  }
}

use Devel::PPPort;
use strict;
$^W = 1;

package Devel::PPPort;
use vars '@ISA';
require DynaLoader;
@ISA = qw(DynaLoader);
bootstrap Devel::PPPort;

package main;

my $rv;

$Devel::PPPort::exception_caught = undef;

$rv = eval { &Devel::PPPort::exception(0) };
ok($@, '');
ok(defined $rv);
ok($rv, 42);
ok($Devel::PPPort::exception_caught, 0);

$Devel::PPPort::exception_caught = undef;

$rv = eval { &Devel::PPPort::exception(1) };
ok($@, "boo\n");
ok(not defined $rv);
ok($Devel::PPPort::exception_caught, 1);

