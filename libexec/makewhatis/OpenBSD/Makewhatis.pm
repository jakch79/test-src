# ex:ts=8 sw=4:
# $OpenBSD: Makewhatis.pm,v 1.1 2004/08/06 12:05:08 espie Exp $
# Copyright (c) 2000-2004 Marc Espie <espie@openbsd.org>
#
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

use strict;
use warnings;

package OpenBSD::Makewhatis;

my ($picky, $testmode);


# $subjects = scan_manpages($list)
#
#   scan a set of manpages, return list of subjects
#
sub scan_manpages
{
    my $list = shift;
    local $_;
    my $done=[];

    for (@$list) {
	my ($file, $subjects);
	if (m/\.(?:Z|gz)$/) {
	    unless (open $file, '-|', "gzip -fdc $_") {
	    	warn "$0: Can't decompress $_\n";
		next;
	    }
	    $_ = $`;
	} else {
	    unless (open $file, '<', $_) {
	    	warn "$0: Can't read $_\n";
		next;
	    }
	}
	if (m/\.[1-9ln][^.]*$/) {
	    require OpenBSD::Makewhatis::Unformated;

	    $subjects = OpenBSD::Makewhatis::Unformated::handle($file, $_);
	} elsif (m/\.0$/) {
	    require OpenBSD::Makewhatis::Formated;

	    $subjects = OpenBSD::Makewhatis::Formated::handle($file, $_);
	    # in test mode, we try harder
	} elsif ($testmode) {
	    require OpenBSD::Makewhatis::Unformated;

	    $subjects = OpenBSD::Makewhatis::Unformated::handle($file, $_);
	    if (@$subjects == 0) {
		require OpenBSD::Makewhatis::Formated;

	    	$subjects = OpenBSD::Makewhatis::Formated::handle($file, $_);
	    }
	} else {
	    print STDERR "Can't find type of $_";
	    next;
	}
	if ($picky) {
		require OpenBSD::Makewhatis::Check;

		for my $s (@$subjects) {
			OpenBSD::Makewhatis::Check::verify_subject($s, $_);
		}
	}
	push @$done, @$subjects;
    }
    return $done;
}

# build_index($dir)
#
#   build index for $dir
#
sub build_index
{
    require OpenBSD::Makewhatis::Find;
    require OpenBSD::Makewhatis::Whatis;

    my $dir = shift;
    my $list = OpenBSD::Makewhatis::Find::find_manpages($dir);
    my $subjects = scan_manpages($list);
    OpenBSD::Makewhatis::Whatis::write($subjects, $dir);
}

sub merge
{
	require OpenBSD::Makewhatis::Whatis;

	my ($mandir, $args) = @_;
	    
	unless (-d $mandir) {
	    die "$0: $mandir: not a directory"
	}
	chdir $mandir;

	my $whatis = "$mandir/whatis.db";
	open(my $old, '<', $whatis) or
	    die "$0 $whatis to merge with";
	my $subjects = scan_manpages($args);
	while (<$old>) {
	    chomp;
	    push(@$subjects, $_);
	}
	close($old);
	OpenBSD::Makewhatis::Whatis::write($subjects, $mandir);
}

sub remove
{
	require OpenBSD::Makewhatis::Whatis;

	my ($mandir, $args) = @_;
	unless (-d $mandir) {
	    die "$0: $mandir: not a directory"
	}
	chdir $mandir;

	my $whatis = "$mandir/whatis.db";
	open(my $old, '<', $whatis) or
	    die "$0 $whatis to merge with";
	my $subjects = scan_manpages($args);
	my %remove = map {$_ => 1 } @$subjects;
	$subjects = [];
	while (<$old>) {
	    chomp;
	    push(@$subjects, $_) unless defined $remove{$_};
	}
	close($old);
	OpenBSD::Makewhatis::Whatis::write($subjects, $mandir);
}

sub default_dirs
{
	local $_;
	my $args=[];
	open(my $conf, '<', '/etc/man.conf') or 
	    die "$0: Can't open /etc/man.conf";
	while (<$conf>) {
	    chomp;
	    push(@$args, $1) if /^_whatdb\s+(.*)\/whatis\.db\s*$/;
	}
	close $conf;
	return $args;
}

sub makewhatis
{
	my ($args, $opts) = @_;
	if (defined $opts->{'p'}) {
	    $picky = 1;
	}
	if (defined $opts->{'t'}) {
	    $testmode = 1;
	    my $subjects = scan_manpages($args);
	    print join("\n", @$subjects), "\n";
	    return;
	} 

	if (defined $opts->{'d'}) {
	    merge($opts->{'d'}, $args);
	    return;
	}
	if (defined $opts->{'u'}) {
	    remove($opts->{'u'}, $args);
	    return;
	}
	if (@$args == 0) {
	    $args = default_dirs();
	}
		
	for my $mandir (@$args) {
	    if (-d $mandir) {
		build_index($mandir);
	    } elsif (-e $mandir || $picky) {
		print STDERR "$0: $mandir is not a directory\n";
	    }
	}
}

1;
