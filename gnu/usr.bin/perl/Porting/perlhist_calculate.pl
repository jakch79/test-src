#!perl
# Generates info for perlhist from this perl tree
# run this from the root of a perl tree
#
# Data is on STDOUT.

use 5.010000;
use File::Find;
use warnings;
use strict;

sub calc_core {

	my @core;
	push @core, glob '*.h';
	push @core, glob '*.c';
	push @core, glob '*.y';
	my $coresize = 0;

	foreach my $corefile (@core) {
		$coresize += (-s $corefile) / 1000;
	}

	my $corenumber = scalar @core;
	$coresize = int $coresize;

	say "core: $coresize $corenumber";
}

sub calc_lib {

	our $libsize = 0;
	our $libnumber = 0;
	sub wanted_lib {
		$File::Find::name =~ /[.] (?:pm|pl)\z/msx or return;
		$libsize += (-s $_) / 1000;
		$libnumber++;
	}

	find(\&wanted_lib, 'lib');
	$libsize = int $libsize;

	say "lib: $libsize $libnumber";
}

sub calc_ext {

	our $extsize = 0;
	our $extnumber = 0;
	sub wanted_ext {
		$File::Find::name =~ /[.] (?:h|c|y|t|xs|pm)\z/msx or return;
		$extsize += (-s $_) / 1000;
		$extnumber++;
	}

	if (-d 'dist') {
		find(\&wanted_ext, 'ext', 'dist', 'cpan');
	} else {
		find(\&wanted_ext, 'ext');
	}

	$extsize = int $extsize;

	say "ext: $extsize $extnumber";
}

sub calc_t {

	our $tsize = 0;
	our $tnumber = 0;
	sub wanted_t {
		$File::Find::name =~ /[.] t\z/msx or return;
		$tsize += (-s $_) / 1000;
		$tnumber++;
	}

	find(\&wanted_t, '.');

	$tsize = int $tsize;

	say "t: $tsize $tnumber";
}

sub calc_doc {

	my @doc;
	push @doc, 'INSTALL';
	push @doc, 'README';
	push @doc, glob 'README.*';
	our $docsize = 0;

	foreach my $docfile (@doc) {
		$docsize += (-s $docfile) / 1000;
	}

	our $docnumber = scalar @doc;

	sub wanted_doc {
		$File::Find::name =~ /[.] pod\z/msx or return;
		$docsize += (-s $_) / 1000;
		$docnumber++;
	}

	find(\&wanted_doc, 'pod');

	$docsize = int $docsize;

	say "doc: $docsize $docnumber";
}

sub calc_dir {
	my $dir = shift;

	return if ! -d $dir;

	our $dirnumber = 0;
	our $dirsize = 0;

	sub wanted_dir {
		$dirsize += (-s $_) / 1000;
		$dirnumber++;
	}

	find(\&wanted_dir, $dir);

	$dirsize = int $dirsize;

	say "$dir: $dirsize $dirnumber";
}


sub calc_longtable {

	print "\n\nTable\n";

	foreach my $dir (qw(beos)) {
		calc_dir($dir);
	}

	my $configure_size = int ((-s 'Configure') / 1000);

	say "Configure: $configure_size 1";

	foreach my $dir (qw(Cross djgpp emacs epoc h2pl hints mad mint mpeix NetWare os2 plan9 Porting qnx symbian utils uts vmesa vms vos win32 x2p)) {
		calc_dir($dir);
	}
}

say "Selected release sizes for perl tarball:";
print "\n";

calc_core();
calc_lib();
calc_ext();
calc_t();
calc_doc();
calc_longtable();

exit;
