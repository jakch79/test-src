package Socket;

our($VERSION, @ISA, @EXPORT, @EXPORT_OK, %EXPORT_TAGS);
$VERSION = "1.72";

=head1 NAME

Socket, sockaddr_in, sockaddr_un, inet_aton, inet_ntoa - load the C socket.h defines and structure manipulators 

=head1 SYNOPSIS

    use Socket;

    $proto = getprotobyname('udp');
    socket(Socket_Handle, PF_INET, SOCK_DGRAM, $proto);
    $iaddr = gethostbyname('hishost.com');
    $port = getservbyname('time', 'udp');
    $sin = sockaddr_in($port, $iaddr);
    send(Socket_Handle, 0, 0, $sin);

    $proto = getprotobyname('tcp');
    socket(Socket_Handle, PF_INET, SOCK_STREAM, $proto);
    $port = getservbyname('smtp', 'tcp');
    $sin = sockaddr_in($port,inet_aton("127.1"));
    $sin = sockaddr_in(7,inet_aton("localhost"));
    $sin = sockaddr_in(7,INADDR_LOOPBACK);
    connect(Socket_Handle,$sin);

    ($port, $iaddr) = sockaddr_in(getpeername(Socket_Handle));
    $peer_host = gethostbyaddr($iaddr, AF_INET);
    $peer_addr = inet_ntoa($iaddr);

    $proto = getprotobyname('tcp');
    socket(Socket_Handle, PF_UNIX, SOCK_STREAM, $proto);
    unlink('/tmp/usock');
    $sun = sockaddr_un('/tmp/usock');
    connect(Socket_Handle,$sun);

=head1 DESCRIPTION

This module is just a translation of the C F<socket.h> file.
Unlike the old mechanism of requiring a translated F<socket.ph>
file, this uses the B<h2xs> program (see the Perl source distribution)
and your native C compiler.  This means that it has a 
far more likely chance of getting the numbers right.  This includes
all of the commonly used pound-defines like AF_INET, SOCK_STREAM, etc.

Also, some common socket "newline" constants are provided: the
constants C<CR>, C<LF>, and C<CRLF>, as well as C<$CR>, C<$LF>, and
C<$CRLF>, which map to C<\015>, C<\012>, and C<\015\012>.  If you do
not want to use the literal characters in your programs, then use
the constants provided here.  They are not exported by default, but can
be imported individually, and with the C<:crlf> export tag:

    use Socket qw(:DEFAULT :crlf);

In addition, some structure manipulation functions are available:

=over

=item inet_aton HOSTNAME

Takes a string giving the name of a host, and translates that
to the 4-byte string (structure). Takes arguments of both
the 'rtfm.mit.edu' type and '18.181.0.24'. If the host name
cannot be resolved, returns undef. For multi-homed hosts (hosts
with more than one address), the first address found is returned.

=item inet_ntoa IP_ADDRESS

Takes a four byte ip address (as returned by inet_aton())
and translates it into a string of the form 'd.d.d.d'
where the 'd's are numbers less than 256 (the normal
readable four dotted number notation for internet addresses).

=item INADDR_ANY

Note: does not return a number, but a packed string.

Returns the 4-byte wildcard ip address which specifies any
of the hosts ip addresses. (A particular machine can have
more than one ip address, each address corresponding to
a particular network interface. This wildcard address
allows you to bind to all of them simultaneously.)
Normally equivalent to inet_aton('0.0.0.0').

=item INADDR_BROADCAST

Note: does not return a number, but a packed string.

Returns the 4-byte 'this-lan' ip broadcast address.
This can be useful for some protocols to solicit information
from all servers on the same LAN cable.
Normally equivalent to inet_aton('255.255.255.255').

=item INADDR_LOOPBACK

Note - does not return a number.

Returns the 4-byte loopback address. Normally equivalent
to inet_aton('localhost').

=item INADDR_NONE

Note - does not return a number.

Returns the 4-byte 'invalid' ip address. Normally equivalent
to inet_aton('255.255.255.255').

=item sockaddr_in PORT, ADDRESS

=item sockaddr_in SOCKADDR_IN

In a list context, unpacks its SOCKADDR_IN argument and returns an array
consisting of (PORT, ADDRESS).  In a scalar context, packs its (PORT,
ADDRESS) arguments as a SOCKADDR_IN and returns it.  If this is confusing,
use pack_sockaddr_in() and unpack_sockaddr_in() explicitly.

=item pack_sockaddr_in PORT, IP_ADDRESS

Takes two arguments, a port number and a 4 byte IP_ADDRESS (as returned by
inet_aton()). Returns the sockaddr_in structure with those arguments
packed in with AF_INET filled in.  For internet domain sockets, this
structure is normally what you need for the arguments in bind(),
connect(), and send(), and is also returned by getpeername(),
getsockname() and recv().

=item unpack_sockaddr_in SOCKADDR_IN

Takes a sockaddr_in structure (as returned by pack_sockaddr_in()) and
returns an array of two elements: the port and the 4-byte ip-address.
Will croak if the structure does not have AF_INET in the right place.

=item sockaddr_un PATHNAME

=item sockaddr_un SOCKADDR_UN

In a list context, unpacks its SOCKADDR_UN argument and returns an array
consisting of (PATHNAME).  In a scalar context, packs its PATHNAME
arguments as a SOCKADDR_UN and returns it.  If this is confusing, use
pack_sockaddr_un() and unpack_sockaddr_un() explicitly.
These are only supported if your system has E<lt>F<sys/un.h>E<gt>.

=item pack_sockaddr_un PATH

Takes one argument, a pathname. Returns the sockaddr_un structure with
that path packed in with AF_UNIX filled in. For unix domain sockets, this
structure is normally what you need for the arguments in bind(),
connect(), and send(), and is also returned by getpeername(),
getsockname() and recv().

=item unpack_sockaddr_un SOCKADDR_UN

Takes a sockaddr_un structure (as returned by pack_sockaddr_un())
and returns the pathname.  Will croak if the structure does not
have AF_UNIX in the right place.

=back

=cut

use Carp;
use warnings::register;

require Exporter;
use XSLoader ();
@ISA = qw(Exporter);
@EXPORT = qw(
	inet_aton inet_ntoa pack_sockaddr_in unpack_sockaddr_in
	pack_sockaddr_un unpack_sockaddr_un
	sockaddr_in sockaddr_un
	INADDR_ANY INADDR_BROADCAST INADDR_LOOPBACK INADDR_NONE
	AF_802
	AF_APPLETALK
	AF_CCITT
	AF_CHAOS
	AF_DATAKIT
	AF_DECnet
	AF_DLI
	AF_ECMA
	AF_GOSIP
	AF_HYLINK
	AF_IMPLINK
	AF_INET
	AF_LAT
	AF_MAX
	AF_NBS
	AF_NIT
	AF_NS
	AF_OSI
	AF_OSINET
	AF_PUP
	AF_SNA
	AF_UNIX
	AF_UNSPEC
	AF_X25
	IOV_MAX
	MSG_BCAST
	MSG_CTLFLAGS
	MSG_CTLIGNORE
	MSG_CTRUNC
	MSG_DONTROUTE
	MSG_DONTWAIT
	MSG_EOF
	MSG_EOR
	MSG_ERRQUEUE
	MSG_FIN
	MSG_MAXIOVLEN
	MSG_MCAST
	MSG_NOSIGNAL
	MSG_OOB
	MSG_PEEK
	MSG_PROXY
	MSG_RST
	MSG_SYN
	MSG_TRUNC
	MSG_URG
	MSG_WAITALL
	PF_802
	PF_APPLETALK
	PF_CCITT
	PF_CHAOS
	PF_DATAKIT
	PF_DECnet
	PF_DLI
	PF_ECMA
	PF_GOSIP
	PF_HYLINK
	PF_IMPLINK
	PF_INET
	PF_LAT
	PF_MAX
	PF_NBS
	PF_NIT
	PF_NS
	PF_OSI
	PF_OSINET
	PF_PUP
	PF_SNA
	PF_UNIX
	PF_UNSPEC
	PF_X25
	SCM_CONNECT
	SCM_CREDENTIALS
	SCM_CREDS
	SCM_RIGHTS
	SCM_TIMESTAMP
	SHUT_RD
	SHUT_RDWR
	SHUT_WR
	SOCK_DGRAM
	SOCK_RAW
	SOCK_RDM
	SOCK_SEQPACKET
	SOCK_STREAM
	SOL_SOCKET
	SOMAXCONN
	SO_ACCEPTCONN
	SO_BROADCAST
	SO_DEBUG
	SO_DONTLINGER
	SO_DONTROUTE
	SO_ERROR
	SO_KEEPALIVE
	SO_LINGER
	SO_OOBINLINE
	SO_RCVBUF
	SO_RCVLOWAT
	SO_RCVTIMEO
	SO_REUSEADDR
	SO_REUSEPORT
	SO_SNDBUF
	SO_SNDLOWAT
	SO_SNDTIMEO
	SO_TYPE
	SO_USELOOPBACK
	UIO_MAXIOV
);

@EXPORT_OK = qw(CR LF CRLF $CR $LF $CRLF

	       IPPROTO_TCP
	       TCP_KEEPALIVE
	       TCP_MAXRT
	       TCP_MAXSEG
	       TCP_NODELAY
	       TCP_STDURG);

%EXPORT_TAGS = (
    crlf    => [qw(CR LF CRLF $CR $LF $CRLF)],
    all     => [@EXPORT, @EXPORT_OK],
);

BEGIN {
    sub CR   () {"\015"}
    sub LF   () {"\012"}
    sub CRLF () {"\015\012"}
}

*CR   = \CR();
*LF   = \LF();
*CRLF = \CRLF();

sub sockaddr_in {
    if (@_ == 6 && !wantarray) { # perl5.001m compat; use this && die
	my($af, $port, @quad) = @_;
	warnings::warn "6-ARG sockaddr_in call is deprecated" 
	    if warnings::enabled();
	pack_sockaddr_in($port, inet_aton(join('.', @quad)));
    } elsif (wantarray) {
	croak "usage:   (port,iaddr) = sockaddr_in(sin_sv)" unless @_ == 1;
        unpack_sockaddr_in(@_);
    } else {
	croak "usage:   sin_sv = sockaddr_in(port,iaddr))" unless @_ == 2;
        pack_sockaddr_in(@_);
    }
}

sub sockaddr_un {
    if (wantarray) {
	croak "usage:   (filename) = sockaddr_un(sun_sv)" unless @_ == 1;
        unpack_sockaddr_un(@_);
    } else {
	croak "usage:   sun_sv = sockaddr_un(filename)" unless @_ == 1;
        pack_sockaddr_un(@_);
    }
}

sub INADDR_ANY 		();
sub INADDR_BROADCAST	();
sub INADDR_LOOPBACK	();
sub INADDR_LOOPBACK	();

sub AF_802		();
sub AF_APPLETALK	();
sub AF_CCITT		();
sub AF_CHAOS		();
sub AF_DATAKIT		();
sub AF_DECnet		();
sub AF_DLI		();
sub AF_ECMA		();
sub AF_GOSIP		();
sub AF_HYLINK		();
sub AF_IMPLINK		();
sub AF_INET		();
sub AF_LAT		();
sub AF_MAX		();
sub AF_NBS		();
sub AF_NIT		();
sub AF_NS		();
sub AF_OSI		();
sub AF_OSINET		();
sub AF_PUP		();
sub AF_SNA		();
sub AF_UNIX		();
sub AF_UNSPEC		();
sub AF_X25		();
sub IOV_MAX		();
sub MSG_BCAST		();
sub MSG_CTLFLAGS	();
sub MSG_CTLIGNORE	();
sub MSG_CTRUNC		();
sub MSG_DONTROUTE	();
sub MSG_DONTWAIT	();
sub MSG_EOF		();
sub MSG_EOR		();
sub MSG_ERRQUEUE	();
sub MSG_FIN		();
sub MSG_MAXIOVLEN	();
sub MSG_MCAST		();
sub MSG_NOSIGNAL	();
sub MSG_OOB		();
sub MSG_PEEK		();
sub MSG_PROXY		();
sub MSG_RST		();
sub MSG_SYN		();
sub MSG_TRUNC		();
sub MSG_URG		();
sub MSG_WAITALL		();
sub PF_802		();
sub PF_APPLETALK	();
sub PF_CCITT		();
sub PF_CHAOS		();
sub PF_DATAKIT		();
sub PF_DECnet		();
sub PF_DLI		();
sub PF_ECMA		();
sub PF_GOSIP		();
sub PF_HYLINK		();
sub PF_IMPLINK		();
sub PF_INET		();
sub PF_LAT		();
sub PF_MAX		();
sub PF_NBS		();
sub PF_NIT		();
sub PF_NS		();
sub PF_OSI		();
sub PF_OSINET		();
sub PF_PUP		();
sub PF_SNA		();
sub PF_UNIX		();
sub PF_UNSPEC		();
sub PF_X25		();
sub SCM_CONNECT		();
sub SCM_CREDENTIALS	();
sub SCM_CREDS		();
sub SCM_RIGHTS		();
sub SCM_TIMESTAMP	();
sub SHUT_RD		();
sub SHUT_RDWR		();
sub SHUT_WR		();
sub SOCK_DGRAM		();
sub SOCK_RAW		();
sub SOCK_RDM		();
sub SOCK_SEQPACKET	();
sub SOCK_STREAM		();
sub SOL_SOCKET		();
sub SOMAXCONN		();
sub SO_ACCEPTCONN	();
sub SO_BROADCAST	();
sub SO_DEBUG		();
sub SO_DONTLINGER	();
sub SO_DONTROUTE	();
sub SO_ERROR		();
sub SO_KEEPALIVE	();
sub SO_LINGER		();
sub SO_OOBINLINE	();
sub SO_RCVBUF		();
sub SO_RCVLOWAT		();
sub SO_RCVTIMEO		();
sub SO_REUSEADDR	();
sub SO_SNDBUF		();
sub SO_SNDLOWAT		();
sub SO_SNDTIMEO		();
sub SO_TYPE		();
sub SO_USELOOPBACK	();
sub UIO_MAXIOV		();

sub AUTOLOAD {
    my($constname);
    ($constname = $AUTOLOAD) =~ s/.*:://;
    my $val = constant($constname, @_ ? $_[0] : 0);
    if ($! != 0) {
	my ($pack,$file,$line) = caller;
	croak "Your vendor has not defined Socket macro $constname, used";
    }
    eval "sub $AUTOLOAD () { $val }";
    goto &$AUTOLOAD;
}

XSLoader::load 'Socket', $VERSION;

1;
