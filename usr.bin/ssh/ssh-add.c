/*
 * Author: Tatu Ylonen <ylo@cs.hut.fi>
 * Copyright (c) 1995 Tatu Ylonen <ylo@cs.hut.fi>, Espoo, Finland
 *                    All rights reserved
 * Adds an identity to the authentication server, or removes an identity.
 *
 * As far as I am concerned, the code I have written for this software
 * can be used freely for any purpose.  Any derived versions of this
 * software must be clearly marked as such, and if the derived work is
 * incompatible with the protocol description in the RFC file, it must be
 * called by a name other than "ssh" or "Secure Shell".
 *
 * SSH2 implementation,
 * Copyright (c) 2000, 2001 Markus Friedl.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "includes.h"
RCSID("$OpenBSD: ssh-add.c,v 1.42 2001/06/26 04:59:59 markus Exp $");

#include <openssl/evp.h>

#include "ssh.h"
#include "rsa.h"
#include "log.h"
#include "xmalloc.h"
#include "key.h"
#include "authfd.h"
#include "authfile.h"
#include "pathnames.h"
#include "readpass.h"

/* we keep a cache of one passphrases */
static char *pass = NULL;
static void
clear_pass(void)
{
	if (pass) {
		memset(pass, 0, strlen(pass));
		xfree(pass);
		pass = NULL;
	}
}

static void
delete_file(AuthenticationConnection *ac, const char *filename)
{
	Key *public;
	char *comment = NULL;

	public = key_load_public(filename, &comment);
	if (public == NULL) {
		printf("Bad key file %s\n", filename);
		return;
	}
	if (ssh_remove_identity(ac, public))
		fprintf(stderr, "Identity removed: %s (%s)\n", filename, comment);
	else
		fprintf(stderr, "Could not remove identity: %s\n", filename);
	key_free(public);
	xfree(comment);
}

/* Send a request to remove all identities. */
static void
delete_all(AuthenticationConnection *ac)
{
	int success = 1;

	if (!ssh_remove_all_identities(ac, 1))
		success = 0;
	/* ignore error-code for ssh2 */
	ssh_remove_all_identities(ac, 2);

	if (success)
		fprintf(stderr, "All identities removed.\n");
	else
		fprintf(stderr, "Failed to remove all identities.\n");
}

static void
add_file(AuthenticationConnection *ac, const char *filename)
{
	struct stat st;
	Key *private;
	char *comment = NULL;
	char msg[1024];

	if (stat(filename, &st) < 0) {
		perror(filename);
		exit(1);
	}
	/* At first, try empty passphrase */
	private = key_load_private(filename, "", &comment);
	if (comment == NULL)
		comment = xstrdup(filename);
	/* try last */
	if (private == NULL && pass != NULL)
		private = key_load_private(filename, pass, NULL);
	if (private == NULL) {
		/* clear passphrase since it did not work */
		clear_pass();
		snprintf(msg, sizeof msg, "Enter passphrase for %.200s: ",
		   comment);
		for (;;) {
			pass = read_passphrase(msg, RP_ALLOW_STDIN);
			if (strcmp(pass, "") == 0) {
				clear_pass();
				xfree(comment);
				return;
			}
			private = key_load_private(filename, pass, &comment);
			if (private != NULL)
				break;
			clear_pass();
			strlcpy(msg, "Bad passphrase, try again: ", sizeof msg);
		}
	}
	if (ssh_add_identity(ac, private, comment))
		fprintf(stderr, "Identity added: %s (%s)\n", filename, comment);
	else
		fprintf(stderr, "Could not add identity: %s\n", filename);
	xfree(comment);
	key_free(private);
}

static void
update_card(AuthenticationConnection *ac, int add, int id)
{
	if (ssh_update_card(ac, add, id))
		fprintf(stderr, "Card %s: %d\n",
		     add ? "added" : "removed", id);
	else
		fprintf(stderr, "Could not %s card: %d\n",
		     add ? "add" : "remove", id);
}

static void
list_identities(AuthenticationConnection *ac, int do_fp)
{
	Key *key;
	char *comment, *fp;
	int had_identities = 0;
	int version;

	for (version = 1; version <= 2; version++) {
		for (key = ssh_get_first_identity(ac, &comment, version);
		     key != NULL;
		     key = ssh_get_next_identity(ac, &comment, version)) {
			had_identities = 1;
			if (do_fp) {
				fp = key_fingerprint(key, SSH_FP_MD5,
				    SSH_FP_HEX);
				printf("%d %s %s (%s)\n",
				    key_size(key), fp, comment, key_type(key));
				xfree(fp);
			} else {
				if (!key_write(key, stdout))
					fprintf(stderr, "key_write failed");
				fprintf(stdout, " %s\n", comment);
			}
			key_free(key);
			xfree(comment);
		}
	}
	if (!had_identities)
		printf("The agent has no identities.\n");
}

static void
usage(void)
{
	printf("Usage: ssh-add [options]\n");
	printf("    -l, -L        : list identities\n");
	printf("    -d            : delete identity\n");
	printf("    -D            : delete all identities\n");
	printf("    -s reader_num : add key in the smartcard in reader_num.\n");
	printf("    -e reader_num : remove key in the smartcard in reader_num.\n");
	exit (1);
}

int
main(int argc, char **argv)
{
	AuthenticationConnection *ac = NULL;
	struct passwd *pw;
	char buf[1024];
	int no_files = 1;
	int i;
	int deleting = 0;
	int sc_mode = 0;
	int sc_reader_num = 0;

	SSLeay_add_all_algorithms();

	/* At first, get a connection to the authentication agent. */
	ac = ssh_get_authentication_connection();
	if (ac == NULL) {
		fprintf(stderr, "Could not open a connection to your authentication agent.\n");
		exit(1);
	}
	for (i = 1; i < argc; i++) {
		if ((strcmp(argv[i], "-l") == 0) ||
		    (strcmp(argv[i], "-L") == 0)) {
			list_identities(ac, argv[i][1] == 'l' ? 1 : 0);
			/* Don't default-add/delete if -l. */
			no_files = 0;
			continue;
		}
		if (strcmp(argv[i], "-d") == 0) {
			deleting = 1;
			continue;
		}
		if (strcmp(argv[i], "-D") == 0) {
			delete_all(ac);
			no_files = 0;
			continue;
		}
		if (strcmp(argv[i], "-s") == 0) {
			sc_mode = 1;
			deleting = 0; 
			i++;
			if (i >= argc)
				usage();
			sc_reader_num = atoi(argv[i]);
			continue; 
		}
		if (strcmp(argv[i], "-e") == 0) {
			sc_mode = 1;
			deleting = 1; 
			i++;
			if (i >= argc)
				usage();
			sc_reader_num = atoi(argv[i]);
			continue; 
		}
		if (sc_mode == 1)
			update_card(ac, !deleting, sc_reader_num);
		no_files = 0;
		if (deleting)
			delete_file(ac, argv[i]);
		else
			add_file(ac, argv[i]);
	}
	if (sc_mode == 1) {
		update_card(ac, !deleting, sc_reader_num);
		ssh_close_authentication_connection(ac);
		exit(0);
	}
	if (no_files) {
		pw = getpwuid(getuid());
		if (!pw) {
			fprintf(stderr, "No user found with uid %u\n",
			    (u_int)getuid());
			ssh_close_authentication_connection(ac);
			exit(1);
		}
		snprintf(buf, sizeof buf, "%s/%s", pw->pw_dir, _PATH_SSH_CLIENT_IDENTITY);
		if (deleting)
			delete_file(ac, buf);
		else
			add_file(ac, buf);
	}
	clear_pass();
	ssh_close_authentication_connection(ac);
	exit(0);
}
