#!./perl

BEGIN {
    chdir 't' if -d 't';
    unshift @INC, '../lib';
}

use Math::BigInt;

$test = 0;
$| = 1;
print "1..278\n";
while (<DATA>) {
       chop;
       if (s/^&//) {
               $f = $_;
       } else {
               ++$test;
               @args = split(/:/,$_,99);
               $ans = pop(@args);
               $try = "\$x = new Math::BigInt \"$args[0]\";";
               if ($f eq "bnorm"){
                   $try .= "\$x+0;";
               } elsif ($f eq "bneg") {
                   $try .= "-\$x;";
               } elsif ($f eq "babs") {
                   $try .= "abs \$x;";
               } else {
                   $try .= "\$y = new Math::BigInt \"$args[1]\";";
                   if ($f eq "bcmp"){
                       $try .= "\$x <=> \$y;";
                   }elsif ($f eq "badd"){
                       $try .= "\$x + \$y;";
                   }elsif ($f eq "bsub"){
                       $try .= "\$x - \$y;";
                   }elsif ($f eq "bmul"){
                       $try .= "\$x * \$y;";
                   }elsif ($f eq "bdiv"){
                       $try .= "\$x / \$y;";
                   }elsif ($f eq "bmod"){
                       $try .= "\$x % \$y;";
                   }elsif ($f eq "bgcd"){
                       $try .= "Math::BigInt::bgcd(\$x, \$y);";
                   }elsif ($f eq "blsft"){
                       $try .= "\$x << \$y;";
                   }elsif ($f eq "brsft"){
                       $try .= "\$x >> \$y;";
                   }elsif ($f eq "band"){
                       $try .= "\$x & \$y;";
                   }elsif ($f eq "bior"){
                       $try .= "\$x | \$y;";
                   }elsif ($f eq "bxor"){
                       $try .= "\$x ^ \$y;";
                   }elsif ($f eq "bnot"){
                       $try .= "~\$x;";
                   } else { warn "Unknown op"; }
               }
               #print ">>>",$try,"<<<\n";
               $ans1 = eval $try;
               if ("$ans1" eq $ans) { #bug!
                       print "ok $test\n";
               } else {
                       print "not ok $test\n";
                       print "# '$try' expected: '$ans' got: '$ans1'\n";
               }
       }
}

{
  use Math::BigInt ':constant';

  $test++;
  print "not " 
    unless 2**150 eq "+1427247692705959881058285969449495136382746624";
  print "ok $test\n";
  $test++;
  @a = ();
  for ($i = 1; $i < 10; $i++) {
    push @a, $i;
  }
  print "not " unless "@a" eq "+1 +2 +3 +4 +5 +6 +7 +8 +9";
  print "ok $test\n";
}
 
__END__
&bnorm
abc:NaN
   1 a:NaN
1bcd2:NaN
11111b:NaN
+1z:NaN
-1z:NaN
0:+0
+0:+0
+00:+0
+0 0 0:+0
000000  0000000   00000:+0
-0:+0
-0000:+0
+1:+1
+01:+1
+001:+1
+00000100000:+100000
123456789:+123456789
-1:-1
-01:-1
-001:-1
-123456789:-123456789
-00000100000:-100000
&bneg
abd:NaN
+0:+0
+1:-1
-1:+1
+123456789:-123456789
-123456789:+123456789
&babs
abc:NaN
+0:+0
+1:+1
-1:+1
+123456789:+123456789
-123456789:+123456789
&bcmp
abc:abc:
abc:+0:
+0:abc:
+0:+0:0
-1:+0:-1
+0:-1:1
+1:+0:1
+0:+1:-1
-1:+1:-1
+1:-1:1
-1:-1:0
+1:+1:0
+123:+123:0
+123:+12:1
+12:+123:-1
-123:-123:0
-123:-12:-1
-12:-123:1
+123:+124:-1
+124:+123:1
-123:-124:1
-124:-123:-1
+100:+5:1
&badd
abc:abc:NaN
abc:+0:NaN
+0:abc:NaN
+0:+0:+0
+1:+0:+1
+0:+1:+1
+1:+1:+2
-1:+0:-1
+0:-1:-1
-1:-1:-2
-1:+1:+0
+1:-1:+0
+9:+1:+10
+99:+1:+100
+999:+1:+1000
+9999:+1:+10000
+99999:+1:+100000
+999999:+1:+1000000
+9999999:+1:+10000000
+99999999:+1:+100000000
+999999999:+1:+1000000000
+9999999999:+1:+10000000000
+99999999999:+1:+100000000000
+10:-1:+9
+100:-1:+99
+1000:-1:+999
+10000:-1:+9999
+100000:-1:+99999
+1000000:-1:+999999
+10000000:-1:+9999999
+100000000:-1:+99999999
+1000000000:-1:+999999999
+10000000000:-1:+9999999999
+123456789:+987654321:+1111111110
-123456789:+987654321:+864197532
-123456789:-987654321:-1111111110
+123456789:-987654321:-864197532
&bsub
abc:abc:NaN
abc:+0:NaN
+0:abc:NaN
+0:+0:+0
+1:+0:+1
+0:+1:-1
+1:+1:+0
-1:+0:-1
+0:-1:+1
-1:-1:+0
-1:+1:-2
+1:-1:+2
+9:+1:+8
+99:+1:+98
+999:+1:+998
+9999:+1:+9998
+99999:+1:+99998
+999999:+1:+999998
+9999999:+1:+9999998
+99999999:+1:+99999998
+999999999:+1:+999999998
+9999999999:+1:+9999999998
+99999999999:+1:+99999999998
+10:-1:+11
+100:-1:+101
+1000:-1:+1001
+10000:-1:+10001
+100000:-1:+100001
+1000000:-1:+1000001
+10000000:-1:+10000001
+100000000:-1:+100000001
+1000000000:-1:+1000000001
+10000000000:-1:+10000000001
+123456789:+987654321:-864197532
-123456789:+987654321:-1111111110
-123456789:-987654321:+864197532
+123456789:-987654321:+1111111110
&bmul
abc:abc:NaN
abc:+0:NaN
+0:abc:NaN
+0:+0:+0
+0:+1:+0
+1:+0:+0
+0:-1:+0
-1:+0:+0
+123456789123456789:+0:+0
+0:+123456789123456789:+0
-1:-1:+1
-1:+1:-1
+1:-1:-1
+1:+1:+1
+2:+3:+6
-2:+3:-6
+2:-3:-6
-2:-3:+6
+111:+111:+12321
+10101:+10101:+102030201
+1001001:+1001001:+1002003002001
+100010001:+100010001:+10002000300020001
+10000100001:+10000100001:+100002000030000200001
+11111111111:+9:+99999999999
+22222222222:+9:+199999999998
+33333333333:+9:+299999999997
+44444444444:+9:+399999999996
+55555555555:+9:+499999999995
+66666666666:+9:+599999999994
+77777777777:+9:+699999999993
+88888888888:+9:+799999999992
+99999999999:+9:+899999999991
&bdiv
abc:abc:NaN
abc:+1:abc:NaN
+1:abc:NaN
+0:+0:NaN
+0:+1:+0
+1:+0:NaN
+0:-1:+0
-1:+0:NaN
+1:+1:+1
-1:-1:+1
+1:-1:-1
-1:+1:-1
+1:+2:+0
+2:+1:+2
+1000000000:+9:+111111111
+2000000000:+9:+222222222
+3000000000:+9:+333333333
+4000000000:+9:+444444444
+5000000000:+9:+555555555
+6000000000:+9:+666666666
+7000000000:+9:+777777777
+8000000000:+9:+888888888
+9000000000:+9:+1000000000
+35500000:+113:+314159
+71000000:+226:+314159
+106500000:+339:+314159
+1000000000:+3:+333333333
+10:+5:+2
+100:+4:+25
+1000:+8:+125
+10000:+16:+625
+999999999999:+9:+111111111111
+999999999999:+99:+10101010101
+999999999999:+999:+1001001001
+999999999999:+9999:+100010001
+999999999999999:+99999:+10000100001
&bmod
abc:abc:NaN
abc:+1:abc:NaN
+1:abc:NaN
+0:+0:NaN
+0:+1:+0
+1:+0:NaN
+0:-1:+0
-1:+0:NaN
+1:+1:+0
-1:-1:+0
+1:-1:+0
-1:+1:+0
+1:+2:+1
+2:+1:+0
+1000000000:+9:+1
+2000000000:+9:+2
+3000000000:+9:+3
+4000000000:+9:+4
+5000000000:+9:+5
+6000000000:+9:+6
+7000000000:+9:+7
+8000000000:+9:+8
+9000000000:+9:+0
+35500000:+113:+33
+71000000:+226:+66
+106500000:+339:+99
+1000000000:+3:+1
+10:+5:+0
+100:+4:+0
+1000:+8:+0
+10000:+16:+0
+999999999999:+9:+0
+999999999999:+99:+0
+999999999999:+999:+0
+999999999999:+9999:+0
+999999999999999:+99999:+0
&bgcd
abc:abc:NaN
abc:+0:NaN
+0:abc:NaN
+0:+0:+0
+0:+1:+1
+1:+0:+1
+1:+1:+1
+2:+3:+1
+3:+2:+1
+100:+625:+25
+4096:+81:+1
&blsft
abc:abc:NaN
+2:+2:+8
+1:+32:+4294967296
+1:+48:+281474976710656
+8:-2:NaN
&brsft
abc:abc:NaN
+8:+2:+2
+4294967296:+32:+1
+281474976710656:+48:+1
+2:-2:NaN
&band
abc:abc:NaN
+8:+2:+0
+281474976710656:+0:+0
+281474976710656:+1:+0
+281474976710656:+281474976710656:+281474976710656
&bior
abc:abc:NaN
+8:+2:+10
+281474976710656:+0:+281474976710656
+281474976710656:+1:+281474976710657
+281474976710656:+281474976710656:+281474976710656
&bxor
abc:abc:NaN
+8:+2:+10
+281474976710656:+0:+281474976710656
+281474976710656:+1:+281474976710657
+281474976710656:+281474976710656:+0
&bnot
abc:NaN
+0:-1
+8:-9
+281474976710656:-281474976710657
