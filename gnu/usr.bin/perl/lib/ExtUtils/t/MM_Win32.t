#!/usr/bin/perl

BEGIN {
    if( $ENV{PERL_CORE} ) {
        chdir 't' if -d 't';
        @INC = '../lib';
    }
    else {
        unshift @INC, 't/lib';
    }
}
chdir 't';

use strict;
use Test::More;

BEGIN {
	if ($^O =~ /MSWin32/i) {
		plan tests => 40;
	} else {
		plan skip_all => 'This is not Win32';
	}
}

use Config;
use File::Spec;
use File::Basename;
use ExtUtils::MM;

require_ok( 'ExtUtils::MM_Win32' );

# Dummy MM object until we have a real MM init method.
my $MM = bless {
                DIR     => [],
                NOECHO  => '@',
                XS      => {},
                MAKEFILE => 'Makefile',
                RM_RF   => 'rm -rf',
                MV      => 'mv',
               }, 'MM';


# replace_manpage_separator() => tr|/|.|s ?
{
    my $man = 'a/path/to//something';
    ( my $replaced = $man ) =~ tr|/|.|s;
    is( $MM->replace_manpage_separator( $man ),
        $replaced, 'replace_manpage_separator()' );
}

# maybe_command()
SKIP: {
    skip( '$ENV{COMSPEC} not set', 2 )
        unless $ENV{COMSPEC} =~ m!((?:[a-z]:)?[^|<>]+)!i;
    my $comspec = $1;
    is( $MM->maybe_command( $comspec ), 
        $comspec, 'COMSPEC is a maybe_command()' );
    ( my $comspec2 = $comspec ) =~ s|\..{3}$||;
    like( $MM->maybe_command( $comspec2 ), 
          qr/\Q$comspec/i, 
          'maybe_command() without extension' );
}

my $had_pathext = exists $ENV{PATHEXT};
{
    local $ENV{PATHEXT} = '.exe';
    ok( ! $MM->maybe_command( 'not_a_command.com' ), 
        'not a maybe_command()' );
}
# Bug in Perl.  local $ENV{FOO} won't delete the key afterward.
delete $ENV{PATHEXT} unless $had_pathext;

# file_name_is_absolute() [Does not support UNC-paths]
{
    ok( $MM->file_name_is_absolute( 'C:/' ), 
        'file_name_is_absolute()' );
    ok( ! $MM->file_name_is_absolute( 'some/path/' ),
        'not file_name_is_absolute()' );

}

# find_perl() 
# Should be able to find running perl... $^X is OK on Win32
{
    my $my_perl = $1 if $^X  =~ /(.*)/; # are we in -T or -t?
    my( $perl, $path ) = fileparse( $my_perl );
    like( $MM->find_perl( $], [ $perl ], [ $path ] ), 
          qr/^\Q$my_perl\E$/i, 'find_perl() finds this perl' );
}

# catdir() (calls MM_Win32->canonpath)
{
    my @path_eg = qw( c: trick dir/now_OK );

    is( $MM->catdir( @path_eg ), 
         'C:\\trick\\dir\\now_OK', 'catdir()' );
    is( $MM->catdir( @path_eg ), 
        File::Spec->catdir( @path_eg ), 
        'catdir() eq File::Spec->catdir()' );

# catfile() (calls MM_Win32->catdir)
    push @path_eg, 'file.ext';

    is( $MM->catfile( @path_eg ),
        'C:\\trick\\dir\\now_OK\\file.ext', 'catfile()' );

    is( $MM->catfile( @path_eg ), 
        File::Spec->catfile( @path_eg ), 
        'catfile() eq File::Spec->catfile()' );
}

# init_others(): check if all keys are created and set?
# qw( TOUCH CHMOD CP RM_F RM_RF MV NOOP TEST_F LD AR LDLOADLIBS DEV_NUL )
{
    my $mm_w32 = bless( {}, 'MM' );
    $mm_w32->init_others();
    my @keys = qw( TOUCH CHMOD CP RM_F RM_RF MV NOOP 
                   TEST_F LD AR LDLOADLIBS DEV_NULL );
    for my $key ( @keys ) {
        ok( $mm_w32->{ $key }, "init_others: $key" );
    }
}

# constants()
{
    my $mm_w32 = bless {
        NAME         => 'TestMM_Win32', 
        VERSION      => '1.00',
        VERSION_FROM => 'TestMM_Win32',
        PM           => { 'MM_Win32.pm' => 1 },
    }, 'MM';

    # XXX Hack until we have a proper init method.
    # Flesh out some necessary keys in the MM object.
    foreach my $key (qw(XS C O_FILES H HTMLLIBPODS HTMLSCRIPTPODS
                        MAN1PODS MAN3PODS PARENT_NAME)) {
        $mm_w32->{$key} = '';
    }
    my $s_PM = join( " \\\n\t", sort keys %{$mm_w32->{PM}} );
    my $k_PM = join( " \\\n\t", %{$mm_w32->{PM}} );

    like( $mm_w32->constants(),
          qr|^NAME\ =\ TestMM_Win32\s+VERSION\ =\ 1\.00.+
             MAKEMAKER\ =\ \Q$INC{'ExtUtils/MakeMaker.pm'}\E\s+
             MM_VERSION\ =\ \Q$ExtUtils::MakeMaker::VERSION\E.+
             VERSION_FROM\ =\ TestMM_Win32.+
             TO_INST_PM\ =\ \Q$s_PM\E\s+
             PM_TO_BLIB\ =\ \Q$k_PM\E
          |xs, 'constants()' );

}

# path()
my $had_path = exists $ENV{PATH};
{
    my @path_eg = ( qw( . .. ), 'C:\\Program Files' );
    local $ENV{PATH} = join ';', @path_eg;
    ok( eq_array( [ $MM->path() ], [ @path_eg ] ),
        'path() [preset]' );
}
# Bug in Perl.  local $ENV{FOO} will not delete key afterwards.
delete $ENV{PATH} unless $had_path;

# static_lib() should look into that
# dynamic_bs() should look into that
# dynamic_lib() should look into that

# clean()
{
    my $clean = $Config{cc} =~ /^gcc/i ? 'dll.base dll.exp' : '*.pdb';
    like( $MM->clean(), qr/^clean ::\s+\Q-$(RM_F) $clean\E\s+$/m,
          'clean() Makefile target' );
}

# perl_archive()
{
    my $libperl = $Config{libperl} || 'libperl.a';
    is( $MM->perl_archive(), File::Spec->catfile('$(PERL_INC)', $libperl ),
	    'perl_archive() should respect libperl setting' );
}

# export_list
{
    my $mm_w32 = bless { BASEEXT => 'someext' }, 'MM';
    is( $mm_w32->export_list(), 'someext.def', 'export_list()' );
}

# canonpath()
{
    my $path = 'c:\\Program Files/SomeApp\\Progje.exe';
    is( $MM->canonpath( $path ), File::Spec->canonpath( $path ),
	    'canonpath() eq File::Spec->canonpath' );
}

# perl_script()
my $script_ext  = '';
my $script_name = 'mm_w32tmp';
SKIP: {
    local *SCRIPT;
    skip( "Can't create temp file: $!", 4 )
        unless open SCRIPT, "> $script_name";
    print SCRIPT <<'EOSCRIPT';
#! perl
__END__
EOSCRIPT
    skip( "Can't write to temp file: $!", 4 )
        unless close SCRIPT;
    # now start tests:
    is( $MM->perl_script( $script_name ), 
        "${script_name}$script_ext", "perl_script ($script_ext)" );

    skip( "Can't rename temp file: $!", 3 )
        unless rename $script_name, "${script_name}.pl";
    $script_ext = '.pl';
    is( $MM->perl_script( $script_name ), 
        "${script_name}$script_ext", "perl_script ($script_ext)" );

    skip( "Can't rename temp file: $!", 2 )
        unless rename "${script_name}$script_ext", "${script_name}.bat";
    $script_ext = '.bat';
    is( $MM->perl_script( $script_name ), 
        "${script_name}$script_ext", "perl_script ($script_ext)" );

    skip( "Can't rename temp file: $!", 1 )
        unless rename "${script_name}$script_ext", "${script_name}.noscript";
    $script_ext = '.noscript';

    isnt( $MM->perl_script( $script_name ),
          "${script_name}$script_ext", 
          "not a perl_script anymore ($script_ext)" );
    is( $MM->perl_script( $script_name ), undef,
        "perl_script ($script_ext) returns empty" );
}
unlink "${script_name}$script_ext" if -f "${script_name}$script_ext";


# pm_to_blib()
{
    like( $MM->pm_to_blib(),
          qr/^pm_to_blib: \Q$(TO_INST_PM)\E.+\Q$(TOUCH) \E\$@\s+$/ms,
          'pm_to_blib' );
}

# tool_autosplit()
{
    my %attribs = ( MAXLEN => 255 );
    like( $MM->tool_autosplit( %attribs ),
          qr/^\#\ Usage:\ \$\(AUTOSPLITFILE\)
             \ FileToSplit\ AutoDirToSplitInto.+
             AUTOSPLITFILE\ =\ \$\(PERLRUN\)\ .+
             \$AutoSplit::Maxlen=$attribs{MAXLEN};
          /xms,
          'tool_autosplit()' );
}

# tools_other()
{
    ( my $mm_w32 = bless { }, 'MM' )->init_others();
        
    my $bin_sh = ( $Config{make} =~ /^dmake/i 
               ? "" : ($Config{sh} || 'cmd /c') . "\n" );
    $bin_sh = "SHELL = $bin_sh" if $bin_sh;

    my $tools = join "\n", map "$_ = $mm_w32->{ $_ }"
    	=> qw(CHMOD CP LD MV NOOP RM_F RM_RF TEST_F TOUCH UMASK_NULL DEV_NULL);

    like( $mm_w32->tools_other(),
          qr/^\Q$bin_sh$tools/m,
          'tools_other()' );
};

# xs_o() should look into that
# top_targets() should look into that

# manifypods()
{
    my $mm_w32 = bless { NOECHO    => '' }, 'MM';
    like( $mm_w32->manifypods(),
          qr/^\nmanifypods :\n\t\$\Q(NOOP)\E\n$/,
          'manifypods() Makefile target' );
}

# dist_ci() should look into that
# dist_core() should look into that

# pasthru()
{
    my $pastru = "PASTHRU = " . ($Config{make} =~ /^nmake/i ? "-nologo" : "");
    is( $MM->pasthru(), $pastru, 'pasthru()' );
}

package FakeOut;

sub TIEHANDLE {
	bless(\(my $scalar), $_[0]);
}

sub PRINT {
	my $self = shift;
	$$self .= shift;
}

__END__

=head1 NAME

MM_Win32.t - Tests for ExtUtils::MM_Win32

=head1 TODO

 - Methods to still be checked:
 # static_lib() should look into that
 # dynamic_bs() should look into that
 # dynamic_lib() should look into that
 # xs_o() should look into that
 # top_targets() should look into that
 # dist_ci() should look into that
 # dist_core() should look into that

=head1 AUTHOR

20011228 Abe Timmerman <abe@ztreet.demon.nl>

=cut
