package ExtUtils::XSSymSet;

use Carp qw( &carp );
use strict;
use vars qw( $VERSION );
$VERSION = '1.0';


sub new { 
  my($pkg,$maxlen,$silent) = @_;
  $maxlen ||= 31;
  $silent ||= 0;
  my($obj) = { '__M@xLen' => $maxlen, '__S!lent' => $silent };
  bless $obj, $pkg;
}


sub trimsym {
  my($self,$name,$maxlen,$silent) = @_;

  unless (defined $maxlen) {
    if (ref $self) { $maxlen ||= $self->{'__M@xLen'}; }
    $maxlen ||= 31;
  }
  unless (defined $silent) {
    if (ref $self) { $silent ||= $self->{'__S!lent'}; }
    $silent ||= 0;
  }
  return $name if (length $name <= $maxlen);

  my $trimmed = $name;
  # First, just try to remove duplicated delimiters
  $trimmed =~ s/__/_/g;
  if (length $trimmed > $maxlen) {
    # Next, all duplicated chars
    $trimmed =~ s/(.)\1+/$1/g;
    if (length $trimmed > $maxlen) {
      my $squeezed = $trimmed;
      my($xs,$prefix,$func) = $trimmed =~ /^(XS_)?(.*)_([^_]*)$/;
      if (length $func <= 12) {  # Try to preserve short function names
        my $frac = int(length $prefix / (length $trimmed - $maxlen) + 0.5);
        my $pat = '([^_])';
        if ($frac > 1) { $pat .= '[^A-Z_]{' . ($frac - 1) . '}'; }
        $prefix =~ s/$pat/$1/g;
        $squeezed = "$xs$prefix" . "_$func";
        if (length $squeezed > $maxlen) {
          $pat =~ s/A-Z//;
          $prefix =~ s/$pat/$1/g;
          $squeezed = "$xs$prefix" . "_$func";
        }
      }
      else { 
        my $frac = int(length $trimmed / (length $trimmed - $maxlen) + 0.5);
        my $pat = '([^_])';
        if ($frac > 1) { $pat .= '[^A-Z_]{' . ($frac - 1) . '}'; }
        $squeezed = "$prefix$func";
        $squeezed =~ s/$pat/$1/g;
        if (length "$xs$squeezed" > $maxlen) {
          $pat =~ s/A-Z//;
          $squeezed =~ s/$pat/$1/g;
        }
        $squeezed = "$xs$squeezed";
      }
      if (length $squeezed <= $maxlen) { $trimmed = $squeezed; }
      else {
        my $frac = int((length $trimmed - $maxlen) / length $trimmed + 0.5);
        my $pat = '(.).{$frac}';
        $trimmed =~ s/$pat/$1/g;
      }
    }
  }
  carp "Warning: long symbol $name\n\ttrimmed to $trimmed\n\t" unless $silent;
  return $trimmed;
}


sub addsym {
  my($self,$sym,$maxlen,$silent) = @_;
  my $trimmed = $self->get_trimmed($sym);

  return $trimmed if defined $trimmed;

  $maxlen ||= $self->{'__M@xLen'} || 31;
  $silent ||= $self->{'__S!lent'} || 0;    
  $trimmed = $self->trimsym($sym,$maxlen,1);
  if (exists $self->{$trimmed}) {
    my($i) = "00";
    $trimmed = $self->trimsym($sym,$maxlen-3,$silent);
    while (exists $self->{"${trimmed}_$i"}) { $i++; }
    carp "Warning: duplicate symbol $trimmed\n\tchanged to ${trimmed}_$i\n\t(original was $sym)\n\t"
      unless $silent;
    $trimmed .= "_$i";
  }
  elsif (not $silent and $trimmed ne $sym) {
    carp "Warning: long symbol $sym\n\ttrimmed to $trimmed\n\t";
  }
  $self->{$trimmed} = $sym;
  $self->{'__N+Map'}->{$sym} = $trimmed;
  $trimmed;
}


sub delsym {
  my($self,$sym) = @_;
  my $trimmed = $self->{'__N+Map'}->{$sym};
  if (defined $trimmed) {
    delete $self->{'__N+Map'}->{$sym};
    delete $self->{$trimmed};
  }
  $trimmed;
}


sub get_trimmed {
  my($self,$sym) = @_;
  $self->{'__N+Map'}->{$sym};
}


sub get_orig {
  my($self,$trimmed) = @_;
  $self->{$trimmed};
}


sub all_orig { (keys %{$_[0]->{'__N+Map'}}); }
sub all_trimmed { (grep { /^\w+$/ } keys %{$_[0]}); }

__END__

=head1 NAME

VMS::XSSymSet - keep sets of symbol names palatable to the VMS linker

=head1 SYNOPSIS

  use VMS::XSSymSet;

  $set = new VMS::XSSymSet;
  while ($sym = make_symbol()) { $set->addsym($sym); }
  foreach $safesym ($set->all_trimmed) {
    print "Processing $safesym (derived from ",$self->get_orig($safesym),")\n";
    do_stuff($safesym);
  }

  $safesym = VMS::XSSymSet->trimsym($onesym);

=head1 DESCRIPTION

Since the VMS linker distinguishes symbols based only on the first 31
characters of their names, it is occasionally necessary to shorten
symbol names in order to avoid collisions.  (This is especially true of
names generated by xsubpp, since prefixes generated by nested package
names can become quite long.)  C<VMS::XSSymSet> provides functions to
shorten names in a consistent fashion, and to track a set of names to
insure that each is unique.  While designed with F<xsubpp> in mind, it
may be used with any set of strings.  

This package supplies the following functions, all of which should be
called as methods.

=over 4

=item new([$maxlen[,$silent]])

Creates an empty C<VMS::XSSymset> set of symbols.  This function may be
called as a static method or via an existing object.  If C<$maxlen> or
C<$silent> are specified, they are used as the defaults for maximum
name length and warning behavior in future calls to addsym() or
trimsym() via this object.

=item addsym($name[,$maxlen[,$silent]])

Creates a symbol name from C<$name>, using the methods described
under trimsym(), which is unique in this set of symbols, and returns
the new name.  C<$name> and its resultant are added to the set, and
any future calls to addsym() specifying the same C<$name> will return
the same result, regardless of the value of C<$maxlen> specified.
Unless C<$silent> is true, warnings are output if C<$name> had to be
trimmed or changed in order to avoid collision with an existing symbol
name.  C<$maxlen> and C<$silent> default to the values specified when
this set of symbols was created.  This method must be called via an
existing object.

=item trimsym($name[,$maxlen[,$silent]])

Creates a symbol name C<$maxlen> or fewer characters long from
C<$name> and returns it. If C<$name> is too long, it first tries to
shorten it by removing duplicate characters, then by periodically
removing non-underscore characters, and finally, if necessary, by
periodically removing characters of any type.  C<$maxlen> defaults
to 31.  Unless C<$silent> is true, a warning is output if C<$name>
is altered in any way.  This function may be called either as a
static method or via an existing object, but in the latter case no
check is made to insure that the resulting name is unique in the
set of symbols.

=item delsym($name)

Removes C<$name> from the set of symbols, where C<$name> is the
original symbol name passed previously to addsym().  If C<$name>
existed in the set of symbols, returns its "trimmed" equivalent,
otherwise returns C<undef>.  This method must be called via an
existing object.

=item get_orig($trimmed)

Returns the original name which was trimmed to C<$trimmed> by a
previous call to addsym(), or C<undef> if C<$trimmed> does not
correspond to a member of this set of symbols.  This method must be
called via an existing object.

=item get_trimmed($name)

Returns the trimmed name which was generated from C<$name> by a
previous call to addsym(), or C<undef> if C<$name> is not a member
of this set of symbols.  This method must be called via an
existing object.

=item all_orig()

Returns a list containing all of the original symbol names
from this set.

=item all_trimmed()

Returns a list containing all of the trimmed symbol names
from this set.

=back

=head1 AUTHOR

Charles Bailey  E<lt>I<bailey@newman.upenn.edu>E<gt>

=head1 REVISION

Last revised 14-Feb-1997, for Perl 5.004.

