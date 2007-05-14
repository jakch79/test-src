# ex:ts=8 sw=4:
# $OpenBSD: Search.pm,v 1.2 2007/05/14 11:22:00 espie Exp $
#
# Copyright (c) 2007 Marc Espie <espie@openbsd.org>
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

package OpenBSD::Search;

package OpenBSD::Search::PkgSpec;
our @ISA=(qw(OpenBSD::Search));

sub match_ref
{
	my ($self, $r) = @_;
	my @l = ();

	for my $subpattern (@{$self->{patterns}}) {
		require OpenBSD::PkgSpec;
		push(@l, OpenBSD::PkgSpec::subpattern_match($subpattern, $r));
	}
	return @l;
}

sub match
{
	my ($self, $o) = @_;
	return $self->match_ref($o->list);
}

sub filter
{
	my ($self, @list) = @_;
	return $self->match_ref(\@list);
}

sub new
{
	my ($class, $pattern) = @_;
	my @l = split /\|/, $pattern;
	bless { patterns => \@l }, $class;
}

package OpenBSD::Search::Stem;
our @ISA=(qw(OpenBSD::Search));

sub new
{
	my ($class, $stem) = @_;

	return bless {stem => $stem}, $class;
}

sub split
{
	my ($class, $pkgname) = @_;
	require OpenBSD::PackageName;

	return $class->new(OpenBSD::PackageName::splitstem($pkgname));
}

sub match
{
	my ($self, $o) = @_;
	return $o->stemlist->find($self->{stem});
}

package OpenBSD::Search::PartialStem;
our @ISA=(qw(OpenBSD::Search::Stem));

sub match
{
	my ($self, $o) = @_;
	return $o->stemlist->find_partial($self->{stem});
}

package OpenBSD::Search::Filter;
our @ISA=(qw(OpenBSD::Search));

sub new
{
	my ($class, $code) = @_;

	return bless {code => $code}, $class;
}

sub filter
{
	my ($self, @l) = @_;
	return &{$self->{code}}(@l);
}

sub keep_most_recent
{
	my $class = shift;
	require OpenBSD::PackageName;
	
	return $class->new(\&OpenBSD::PackageName::keep_most_recent);
}

1;
