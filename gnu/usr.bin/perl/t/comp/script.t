#!./perl

# $RCSfile: script.t,v $$Revision: 1.1 $$Date: 1996/08/19 10:13:12 $

print "1..3\n";

$x = `./perl -e 'print "ok\n";'`;
if ($x =~ /DCL-W-NOCOMD/) { $x = `\$ mcr sys\$disk:[]perl. -e "print ""ok\n""";`; }

if ($x eq "ok\n") {print "ok 1\n";} else {print "not ok 1\n";}

open(try,">Comp.script") || (die "Can't open temp file.");
print try 'print "ok\n";'; print try "\n";
close try;

$x = `./perl Comp.script`;
if ($x =~ /DCL-W-NOCOMD/) { $x = `\$ mcr sys\$disk:[]perl. Comp.script`; }

if ($x eq "ok\n") {print "ok 2\n";} else {print "not ok 2\n";}

$x = `./perl <Comp.script`;
if ($x =~ /DCL-W-NOCOMD/) { $x = `\$ mcr sys\$disk:[]perl. <Comp.script`; }

if ($x eq "ok\n") {print "ok 3\n";} else {print "not ok 3\n";}

unlink 'Comp.script' || `/bin/rm -f Comp.script`;
