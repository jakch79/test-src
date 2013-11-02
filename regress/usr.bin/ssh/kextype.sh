#	$OpenBSD: kextype.sh,v 1.2 2013/11/02 22:39:53 markus Exp $
#	Placed in the Public Domain.

tid="login with different key exchange algorithms"

TIME=/usr/bin/time
cp $OBJ/sshd_proxy $OBJ/sshd_proxy_bak
cp $OBJ/ssh_proxy $OBJ/ssh_proxy_bak

kextypes="ecdh-sha2-nistp256 ecdh-sha2-nistp384 ecdh-sha2-nistp521"
kextypes="$kextypes diffie-hellman-group-exchange-sha256"
kextypes="$kextypes diffie-hellman-group-exchange-sha1"
kextypes="$kextypes diffie-hellman-group14-sha1"
kextypes="$kextypes diffie-hellman-group1-sha1"
kextypes="$kextypes curve25519-sha256@libssh.org"

tries="1 2 3 4"
for k in $kextypes; do 
	verbose "kex $k"
	for i in $tries; do
		${SSH} -F $OBJ/ssh_proxy -o KexAlgorithms=$k x true
		if [ $? -ne 0 ]; then
			fail "ssh kex $k"
		fi
	done
done

