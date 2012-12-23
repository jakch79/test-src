#!/bin/ksh -
#
# $OpenBSD: sysmerge.sh,v 1.95 2012/12/23 11:50:19 rpe Exp $
#
# Copyright (c) 2008, 2009, 2010, 2011, 2012 Antoine Jacoutot <ajacoutot@openbsd.org>
# Copyright (c) 1998-2003 Douglas Barton <DougB@FreeBSD.org>
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
#

umask 0022

unset AUTO_INSTALLED_FILES BATCHMODE DIFFMODE ETCSUM NEED_NEWALIASES
unset NEWGRP NEWUSR NEED_REBOOT SRCDIR SRCSUM TGZ TGZURL XETCSUM
unset XTGZ XTGZURL

WRKDIR=$(mktemp -d -p ${TMPDIR:=/var/tmp} sysmerge.XXXXXXXXXX) || exit 1
SWIDTH=$(stty size | awk '{w=$2} END {if (w==0) {w=80} print w}')
MERGE_CMD="${MERGE_CMD:=sdiff -as -w ${SWIDTH} -o}"
FETCH_CMD="${FETCH_CMD:=/usr/bin/ftp -V -m -k ${FTP_KEEPALIVE:-0}}"
REPORT="${REPORT:=${WRKDIR}/sysmerge.log}"
DBDIR="${DBDIR:=/var/db/sysmerge}"

PAGER="${PAGER:=/usr/bin/more}"

# clean leftovers created by make in src
clean_src() {
	[ -n "${SRCDIR}" ] && \
		cd ${SRCDIR}/gnu/usr.sbin/sendmail/cf/cf && make cleandir >/dev/null
}

# restore files from backups or remove the newly generated sum files if
# they did not exist
restore_bak() {
	local i _i
	for i in ${DESTDIR}/${DBDIR}/.{${SRCSUM},${ETCSUM},${XETCSUM}}.bak; do
		_i=$(basename ${i} .bak)
		if [ -f "${i}" ]; then
			mv ${i} ${DESTDIR}/${DBDIR}/${_i#.}
		elif [ -f "${DESTDIR}/${DBDIR}/${_i#.}" ]; then
			rm ${DESTDIR}/${DBDIR}/${_i#.}
		fi
	done
}

usage() {
	echo "usage: ${0##*/} [-bd] [-s [src | etcXX.tgz]] [-x xetcXX.tgz]" >&2
}

warn() {
	echo "\t*** WARNING: $@"
}

error() {
	echo "\t*** ERROR: $@"
}

report() {
	echo "$@" >> ${REPORT}
}

# remove newly created work directory and exit with status 1
error_rm_wrkdir() {
	(($#)) && error "$@"
	rmdir ${WRKDIR} 2>/dev/null
	exit 1
}

trap "restore_bak; clean_src; rm -rf ${WRKDIR}; exit 1" 1 2 3 13 15

if (($(id -u) != 0)); then
	error "need root privileges to run this script"
	usage
	error_rm_wrkdir
fi

do_populate() {
	local cf i _array _d _r _D _E _R _X CF_DIFF CF_FILES CURSUM IGNORE_FILES
	mkdir -p ${DESTDIR}/${DBDIR} || error_rm_wrkdir
	echo "===> Populating temporary root under ${TEMPROOT}"
	mkdir -p ${TEMPROOT}
	if [ -n "${SRCDIR}" ]; then
		SRCSUM=srcsum
		cd ${SRCDIR}/etc
		make DESTDIR=${TEMPROOT} distribution-etc-root-var >/dev/null 2>&1
		(cd ${TEMPROOT} && find . -type f | xargs cksum > ${WRKDIR}/${SRCSUM})
	fi

	if [ -n "${TGZ}" -o -n "${XTGZ}" ]; then
		for i in ${TGZ} ${XTGZ}; do
			tar -xzphf ${i} -C ${TEMPROOT};
		done
		if [ -n "${TGZ}" ]; then
			ETCSUM=etcsum
			_E=$(cd $(dirname ${TGZ}) && pwd)/$(basename ${TGZ})
			(cd ${TEMPROOT} && tar -tzf ${_E} | xargs cksum > ${WRKDIR}/${ETCSUM})
		fi
		if [ -n "${XTGZ}" ]; then
			XETCSUM=xetcsum
			_X=$(cd $(dirname ${XTGZ}) && pwd)/$(basename ${XTGZ})
			(cd ${TEMPROOT} && tar -tzf ${_X} | xargs cksum > ${WRKDIR}/${XETCSUM})
		fi
	fi

	for i in ${SRCSUM} ${ETCSUM} ${XETCSUM}; do
		if [ -f ${DESTDIR}/${DBDIR}/${i} ]; then
			# delete file in temproot if it has not changed since last release
			# and is present in current installation
			if [ -z "${DIFFMODE}" ]; then
				_R=$(cd ${TEMPROOT} && \
					cksum -c ${DESTDIR}/${DBDIR}/${i} 2>/dev/null | grep OK | awk '{ print $2 }' | sed 's/[:]//')
				for _r in ${_R}; do
					if [ -f ${DESTDIR}/${_r} -a -f ${TEMPROOT}/${_r} ]; then
						# sanity check: _always_ compare master.passwd(5) and group(5)
						# we don't want to have missing system user(s) and/or group(s)
						[ ${_r} != ./etc/master.passwd -a ${_r} != ./etc/group ] && \
							rm -f ${TEMPROOT}/${_r}
					fi
				done
			fi

			# set auto-upgradable files
			_D=$(diff -u ${WRKDIR}/${i} ${DESTDIR}/${DBDIR}/${i} | grep -E '^\+' | sed '1d' | awk '{print $3}')
			for _d in ${_D}; do
				CURSUM=$(cd ${DESTDIR:=/} && cksum ${_d} 2>/dev/null)
				[ -n "$(grep "${CURSUM}" ${DESTDIR}/${DBDIR}/${i})" -a -z "$(grep "${CURSUM}" ${WRKDIR}/${i})" ] && \
					_array="${_array} ${_d}"
			done
			[ -n "${_array}" ] && set -A AUTO_UPG -- ${_array}

			mv ${DESTDIR}/${DBDIR}/${i} ${DESTDIR}/${DBDIR}/.${i}.bak
		fi
		mv ${WRKDIR}/${i} ${DESTDIR}/${DBDIR}/${i}
	done

	# files we don't want/need to deal with
	IGNORE_FILES="/etc/*.db
		      /etc/mail/*.db
		      /etc/passwd
		      /etc/motd
		      /etc/myname
		      /var/db/locate.database
		      /var/db/sysmerge/{etc,xetc}sum
		      /var/games/tetris.scores
		      /var/mail/root"
	CF_FILES="/etc/mail/localhost.cf /etc/mail/sendmail.cf /etc/mail/submit.cf"
	for cf in ${CF_FILES}; do
		CF_DIFF=$(diff -q -I "##### " ${TEMPROOT}/${cf} ${DESTDIR}/${cf} 2>/dev/null)
		[ -z "${CF_DIFF}" ] && IGNORE_FILES="${IGNORE_FILES} ${cf}"
	done
	if [ -r /etc/sysmerge.ignore ]; then
		while read i; do \
			IGNORE_FILES="${IGNORE_FILES} $(echo ${i} | sed -e 's,\.\.,,g' -e 's,#.*,,g')"
		done < /etc/sysmerge.ignore
	fi
	for i in ${IGNORE_FILES}; do
		rm -rf ${TEMPROOT}/${i};
	done
}

do_install_and_rm() {
	if [ -f "${5}/${4##*/}" ]; then
		mkdir -p ${BKPDIR}/${4%/*}
		cp ${5}/${4##*/} ${BKPDIR}/${4%/*}
	fi

	if ! install -m "${1}" -o "${2}" -g "${3}" "${4}" "${5}" 2>/dev/null; then
		rm -f ${BKPDIR}/${4%/*}/${4##*/}
		return 1
	fi
	rm -f "${4}"
}

mm_install() {
	local DIR_MODE FILE_MODE FILE_OWN FILE_GRP INSTDIR
	INSTDIR=${1#.}
	INSTDIR=${INSTDIR%/*}

	[ -z "${INSTDIR}" ] && INSTDIR=/

	DIR_MODE=$(stat -f "%OMp%OLp" "${TEMPROOT}/${INSTDIR}")
	eval $(stat -f "FILE_MODE=%OMp%OLp FILE_OWN=%Su FILE_GRP=%Sg" ${1})

	[ -n "${DESTDIR}${INSTDIR}" -a ! -d "${DESTDIR}${INSTDIR}" ] && \
		install -d -o root -g wheel -m "${DIR_MODE}" "${DESTDIR}${INSTDIR}"

	do_install_and_rm "${FILE_MODE}" "${FILE_OWN}" "${FILE_GRP}" "${1}" "${DESTDIR}${INSTDIR}" || return

	case "${1#.}" in
	/dev/MAKEDEV)
		echo " (running MAKEDEV(8))"
		(cd ${DESTDIR}/dev && /bin/sh MAKEDEV all)
		export NEED_REBOOT=1
		;;
	/etc/login.conf)
		if [ -f ${DESTDIR}/etc/login.conf.db ]; then
			echo " (running cap_mkdb(1))"
			cap_mkdb ${DESTDIR}/etc/login.conf
		else
			echo ""
		fi
		export NEED_REBOOT=1
		;;
	/etc/mail/access|/etc/mail/genericstable|/etc/mail/mailertable|/etc/mail/virtusertable)
		echo " (running makemap(8))"
		/usr/libexec/sendmail/makemap hash ${DESTDIR}/${1#.} < ${DESTDIR}/${1#.}
		;;
	/etc/mail/aliases)
		echo " (running newaliases(8))"
		if [ -n "${DESTDIR}" ]; then
			chroot ${DESTDIR} newaliases >/dev/null || export NEED_NEWALIASES=1
		else
			newaliases >/dev/null
		fi
		;;
	/etc/master.passwd)
		echo " (running pwd_mkdb(8))"
		pwd_mkdb -d ${DESTDIR}/etc -p ${DESTDIR}/etc/master.passwd
		;;
	*)
		echo ""
		;;
	esac
}

mm_install_link() {
	local _LINKF _LINKT DIR_MODE 
	_LINKT=$(readlink ${COMPFILE})
	_LINKF=$(dirname ${DESTDIR}${COMPFILE#.})

	DIR_MODE=$(stat -f "%OMp%OLp" "${TEMPROOT}/${_LINKF}")
	[ ! -d "${_LINKF}" ] && \
		install -d -o root -g wheel -m "${DIR_MODE}" "${_LINKF}"

	rm -f ${COMPFILE}
	(cd ${_LINKF} && ln -sf ${_LINKT} .)
	return
}

merge_loop() {
	local INSTALL_MERGED MERGE_AGAIN
	[ "$(expr "${MERGE_CMD}" : ^sdiff.*)" -gt 0 ] && \
		echo "===> Type h at the sdiff prompt (%) to get usage help\n"
	MERGE_AGAIN=1
	while [ -n "${MERGE_AGAIN}" ]; do
		cp -p "${COMPFILE}" "${COMPFILE}.merged"
		${MERGE_CMD} "${COMPFILE}.merged" \
			"${DESTDIR}${COMPFILE#.}" "${COMPFILE}"
		INSTALL_MERGED=v
		while [ "${INSTALL_MERGED}" = "v" ]; do
			echo ""
			echo "  Use 'e' to edit the merged file"
			echo "  Use 'i' to install the merged file"
			echo "  Use 'n' to view a diff between the merged and new files"
			echo "  Use 'o' to view a diff between the old and merged files"
			echo "  Use 'r' to re-do the merge"
			echo "  Use 'v' to view the merged file"
			echo "  Use 'x' to delete the merged file and go back to previous menu"
			echo "  Default is to leave the temporary file to deal with by hand"
			echo ""
			echo -n "===> How should I deal with the merged file? [Leave it for later] "
			read INSTALL_MERGED
			case "${INSTALL_MERGED}" in
			[eE])
				echo "editing merged file...\n"
				if [ -z "${VISUAL}" ]; then
					EDIT="${EDITOR:=/usr/bin/vi}"
				else
					EDIT="${VISUAL}"
				fi
				${EDIT} ${COMPFILE}.merged
				INSTALL_MERGED=v
				;;
			[iI])
				mv "${COMPFILE}.merged" "${COMPFILE}"
				echo -n "\n===> Merging ${COMPFILE#.}"
				mm_install "${COMPFILE}" || \
					warn "problem merging ${COMPFILE#.}"
				unset MERGE_AGAIN
				;;
			[nN])
				(
					echo "comparison between merged and new files:\n"
					diff -u ${COMPFILE}.merged ${COMPFILE}
				) | ${PAGER}
				INSTALL_MERGED=v
				;;
			[oO])
				(
					echo "comparison between old and merged files:\n"
					diff -u ${DESTDIR}${COMPFILE#.} ${COMPFILE}.merged
				) | ${PAGER}
				INSTALL_MERGED=v
				;;
			[rR])
				rm "${COMPFILE}.merged"
				;;
			[vV])
				${PAGER} "${COMPFILE}.merged"
				;;
			[xX])
				rm "${COMPFILE}.merged"
				return 1
				;;
			'')
				echo "===> ${COMPFILE} will remain for your consideration"
				unset MERGE_AGAIN
				;;
			*)
				echo "invalid choice: ${INSTALL_MERGED}"
				INSTALL_MERGED=v
				;;
			esac
		done
	done
}

diff_loop() {
	local i _g _gid _merge_pwd _merge_grp _u CAN_INSTALL HANDLE_COMPFILE NO_INSTALLED
	if [ -n "${BATCHMODE}" ]; then
		HANDLE_COMPFILE=todo
	else
		HANDLE_COMPFILE=v
	fi

	unset NO_INSTALLED
	unset CAN_INSTALL
	unset FORCE_UPG

	while [ "${HANDLE_COMPFILE}" = "v" -o "${HANDLE_COMPFILE}" = "todo" ]; do
		if [ -f "${DESTDIR}${COMPFILE#.}" -a -f "${COMPFILE}" -a -z "${IS_LINK}" ]; then
			if [ -z "${DIFFMODE}" ]; then
				# automatically install files if current != new and current = old
				for i in "${AUTO_UPG[@]}"; do
					[ "${i}" = "${COMPFILE}" ] && FORCE_UPG=1
				done
				# automatically install files which differ only by CVS Id or that are binaries
				if [ -z "$(diff -q -I'[$]OpenBSD:.*$' "${DESTDIR}${COMPFILE#.}" "${COMPFILE}")" -o -n "${FORCE_UPG}" -o -n "${IS_BINFILE}" ]; then
					echo -n "===> Updating ${COMPFILE#.}"
					if mm_install "${COMPFILE}"; then
						AUTO_INSTALLED_FILES="${AUTO_INSTALLED_FILES}${DESTDIR}${COMPFILE#.}\n"
					else
						warn "problem updating ${COMPFILE#.}"
					fi
					return
				fi
				# automatically install missing users
				if [ "${COMPFILE}" = "./etc/master.passwd" ]; then
					while read l; do
						_u=$(echo ${l} | awk -F ':' '{ print $1 }')
						if [ "${_u}" != "root" ]; then
							if [ -z "$(grep -E "^${_u}:" ${DESTDIR}${COMPFILE#.})" ]; then
								echo "===> Adding the ${_u} user"
								if [ -n "${DESTDIR}" ]; then
									chroot ${DESTDIR} chpass -la "${l}"
								else
									chpass -la "${l}"
								fi
								if (($? == 0)); then
									set -A NEWUSR -- ${NEWUSR[@]} ${_u}
								else
									_merge_pwd=1
								fi
							fi
						fi
					done < ${COMPFILE}
					if [ -z ${_merge_pwd} ]; then
						rm "${TEMPROOT}${COMPFILE#.}"
						return
					fi
				fi
				# automatically install missing groups
				if [ "${COMPFILE}" = "./etc/group" ]; then
					while read l; do
						_g=$(echo ${l} | awk -F ':' '{ print $1 }')
						_gid=$(echo ${l} | awk -F ':' '{ print $3 }')
						if [ -z "$(grep -E "^${_g}:" ${DESTDIR}${COMPFILE#.})" ]; then
							echo "===> Adding the ${_g} group"
							if [ -n "${DESTDIR}" ]; then
								chroot ${DESTDIR} groupadd -g "${_gid}" "${_g}"
							else
								groupadd -g "${_gid}" "${_g}"
							fi
							if (($? == 0)); then
								set -A NEWGRP -- ${NEWGRP[@]} ${_g}
							else
								_merge_grp=1
							fi
						fi
					done < ${COMPFILE}
					if [ -z ${_merge_grp} ]; then
						rm "${TEMPROOT}${COMPFILE#.}"
						return
					fi
				fi
			fi
			if [ "${HANDLE_COMPFILE}" = "v" ]; then
				(
					echo "\n========================================================================\n"
					echo "===> Displaying differences between ${COMPFILE} and installed version:"
					echo ""
					diff -u "${DESTDIR}${COMPFILE#.}" "${COMPFILE}"
				) | ${PAGER}
				echo ""
			fi
		else
			# file does not exist on the target system
			if [ -n "${IS_LINK}" ]; then
				if [ -n "${DIFFMODE}" ]; then
					echo ""
					NO_INSTALLED=1
				else
					if mm_install_link; then
						echo "===> ${COMPFILE#.} link created successfully"
						AUTO_INSTALLED_FILES="${AUTO_INSTALLED_FILES}${DESTDIR}${COMPFILE#.}\n"
					else
						warn "problem creating ${COMPFILE#.} link"
					fi
					return
				fi
			fi
			if [ -n "${DIFFMODE}" ]; then
				echo ""
				NO_INSTALLED=1
			else
				echo -n "===> Installing ${COMPFILE#.}"
				if mm_install "${COMPFILE}"; then
					AUTO_INSTALLED_FILES="${AUTO_INSTALLED_FILES}${DESTDIR}${COMPFILE#.}\n"
				else
					warn "problem installing ${COMPFILE#.}"
				fi
				return
			fi
		fi

		if [ -z "${BATCHMODE}" ]; then
			echo "  Use 'd' to delete the temporary ${COMPFILE}"
			if [ "${COMPFILE}" != "./etc/master.passwd" -a "${COMPFILE}" != "./etc/group" -a "${COMPFILE}" != "./etc/hosts" ]; then
				CAN_INSTALL=1
				echo "  Use 'i' to install the temporary ${COMPFILE}"
			fi
			if [ -z "${NO_INSTALLED}" -a -z "${IS_BINFILE}" -a -z "${IS_LINK}" ]; then
				echo "  Use 'm' to merge the temporary and installed versions"
				echo "  Use 'v' to view the diff results again"
			fi
			echo ""
			echo "  Default is to leave the temporary file to deal with by hand"
			echo ""
			echo -n "How should I deal with this? [Leave it for later] "
			read HANDLE_COMPFILE
		else
			unset HANDLE_COMPFILE
		fi

		case "${HANDLE_COMPFILE}" in
		[dD])
			rm "${COMPFILE}"
			echo "\n===> Deleting ${COMPFILE}"
			;;
		[iI])
			if [ -n "${CAN_INSTALL}" ]; then
				echo ""
				if [ -n "${IS_LINK}" ]; then
					if mm_install_link; then
						echo "===> ${COMPFILE#.} link created successfully"
						MERGED_FILES="${MERGED_FILES}${DESTDIR}${COMPFILE#.}\n"
					else
						warn "problem creating ${COMPFILE#.} link"
					fi
				else
					echo -n "===> Updating ${COMPFILE#.}"
					if mm_install "${COMPFILE}"; then
						MERGED_FILES="${MERGED_FILES}${DESTDIR}${COMPFILE#.}\n"
					else
						warn "problem updating ${COMPFILE#.}"
					fi
				fi
			else
				echo "invalid choice: ${HANDLE_COMPFILE}\n"
				HANDLE_COMPFILE="todo"
			fi
				
			;;
		[mM])
			if [ -z "${NO_INSTALLED}" -a -z "${IS_BINFILE}" -a -z "${IS_LINK}" ]; then
				merge_loop && \
					MERGED_FILES="${MERGED_FILES}${DESTDIR}${COMPFILE#.}\n" || \
					HANDLE_COMPFILE="todo"
			else
				echo "invalid choice: ${HANDLE_COMPFILE}\n"
				HANDLE_COMPFILE="todo"
			fi
			;;
		[vV])
			if [ -z "${NO_INSTALLED}" -a -z "${IS_BINFILE}" -a -z "${IS_LINK}" ]; then
				HANDLE_COMPFILE="v"
			else
				echo "invalid choice: ${HANDLE_COMPFILE}\n"
				HANDLE_COMPFILE="todo"
			fi
			;;
		'')
			echo "\n===> ${COMPFILE} will remain for your consideration"
			;;
		*)
			echo "invalid choice: ${HANDLE_COMPFILE}\n"
			HANDLE_COMPFILE="todo"
			continue
			;;
		esac
	done
}

do_compare() {
	local _c1 _c2 COMPFILE CVSID1 CVSID2
	echo "===> Starting comparison"

	cd ${TEMPROOT} || error_rm_wrkdir

	# use -size +0 to avoid comparing empty log files and device nodes;
	# however, we want to keep the symlinks; group and master.passwd
	# need to be handled first in case mm_install needs a new user/group
	_c1="./etc/group ./etc/master.passwd"
	_c2=$(find . -type f -size +0 -or -type l | grep -vE '(./etc/group|./etc/master.passwd)')
	for COMPFILE in ${_c1} ${_c2}; do
		unset IS_BINFILE
		unset IS_LINK
		# links need to be treated in a different way
		[ -h "${COMPFILE}" ] && IS_LINK=1
		if [ ! -e "${DESTDIR}${COMPFILE#.}" ]; then
			diff_loop
			continue
		fi

		# compare CVS $Id's first so if the file hasn't been modified,
		# it will be deleted from temproot and ignored from comparison.
		# several files are generated from scripts so CVS ID is not a
		# reliable way of detecting changes; leave for a full diff.
		if [ -z "${DIFFMODE}" -a "${COMPFILE}" != "./etc/fbtab" \
		    -a "${COMPFILE}" != "./etc/login.conf" \
		    -a "${COMPFILE}" != "./etc/sysctl.conf" \
		    -a "${COMPFILE}" != "./etc/ttys" -a -z "${IS_LINK}" ]; then
			CVSID1=$(grep "[$]OpenBSD:" ${DESTDIR}${COMPFILE#.} 2>/dev/null)
			CVSID2=$(grep "[$]OpenBSD:" ${COMPFILE} 2>/dev/null) || CVSID2=none
			[ "${CVSID2}" = "${CVSID1}" ] && rm "${COMPFILE}"
		fi

		if [ -f "${COMPFILE}" -a -z "${IS_LINK}" ]; then
			# make sure files are different; if not, delete the one in temproot
			if diff -q "${DESTDIR}${COMPFILE#.}" "${COMPFILE}" >/dev/null 2>&1; then
				rm "${COMPFILE}"
			# xetcXX.tgz contains binary files; set IS_BINFILE to disable sdiff
			elif diff -q "${DESTDIR}${COMPFILE#.}" "${COMPFILE}" | grep "Binary" >/dev/null 2>&1; then
				IS_BINFILE=1
				diff_loop
			else
				diff_loop
			fi
		fi
	done

	echo "===> Comparison complete"
}

do_post() {
	local FILES_IN_TEMPROOT FILES_IN_BKPDIR
	echo "===> Checking directory hierarchy permissions (running mtree(8))"
	mtree -qdef ${DESTDIR}/etc/mtree/4.4BSD.dist -p ${DESTDIR:=/} -U >/dev/null
	[ -n "${XTGZ}" ] && \
		mtree -qdef ${DESTDIR}/etc/mtree/BSD.x11.dist -p ${DESTDIR:=/} -U >/dev/null

	if [ -n "${NEED_NEWALIASES}" ]; then
		report "===> A new ${DESTDIR}/etc/mail/aliases file was installed."
		report "However ${DESTDIR}/usr/bin/newaliases could not be run,"
		report "you will need to rebuild your aliases database manually.\n"
	fi

	FILES_IN_TEMPROOT=$(find ${TEMPROOT} -type f ! -name \*.merged -size +0 2>/dev/null)
	FILES_IN_BKPDIR=$(find ${BKPDIR} -type f -size +0 2>/dev/null)
	if [ -n "${AUTO_INSTALLED_FILES}" ]; then
		report "===> Automatically installed file(s)"
		report "${AUTO_INSTALLED_FILES}"
	fi
	if [ -n "${MERGED_FILES}" ]; then
		report "===> Manually merged/installed file(s)"
		report "${MERGED_FILES}"
	fi
	if [ -n "${FILES_IN_BKPDIR}" ]; then
		report "===> Backup of replaced file(s) can be found under"
		report "${BKPDIR}\n"
	fi
	if [ -n "${NEWUSR}" -o -n "${NEWGRP}" ]; then
		report "===> The following user(s)/group(s) have been added"
		[ -n "${NEWUSR}" ] && report "user(s): ${NEWUSR[@]}"
		[ -n "${NEWGRP}" ] && report "group(s): ${NEWGRP[@]}"
		report ""
	fi
	if [ -n "${FILES_IN_TEMPROOT}" ]; then
		report "===> File(s) remaining for you to merge by hand"
		report "${FILES_IN_TEMPROOT}"
	fi

	if [ -e "${REPORT}" ]; then
		echo "===> Output log available at ${REPORT}"
	else
		echo "===> Removing ${WRKDIR}"
		rm -rf "${WRKDIR}"
	fi

	[ -n "${FILES_IN_TEMPROOT}" ] && \
		warn "some files are still left for comparison"

	[ -n "${NEED_NEWALIASES}" ] && \
		warn "newaliases(8) failed to run properly"

	[ -n "${NEED_REBOOT}" ] && \
		warn "some new/updated file(s) may require a reboot"

	unset NEED_NEWALIASES NEED_REBOOT

	clean_src
	rm -f ${DESTDIR}/${DBDIR}/.*.bak
}

while getopts bds:x: arg; do
	case ${arg} in
	b)
		BATCHMODE=1
		;;
	d)
		DIFFMODE=1
		;;
	s)
		if [ -d "${OPTARG}" ]; then
			SRCDIR=${OPTARG}
		elif echo ${OPTARG} | \
		    grep -qE '^(file|ftp|http|https)://.*/etc[0-9][0-9]\.tgz$'; then
			TGZ=${WRKDIR}/etc.tgz
			TGZURL=${OPTARG}
			${FETCH_CMD} -o ${TGZ} ${TGZURL} || \
				error_rm_wrkdir "could not retrieve ${TGZURL}"
		else
			TGZ=${OPTARG}
		fi
		;;
	x)
		if echo ${OPTARG} | \
		    grep -qE '^(file|ftp|http|https)://.*/xetc[0-9][0-9]\.tgz$'; then
			XTGZ=${WRKDIR}/xetc.tgz
			XTGZURL=${OPTARG}
			${FETCH_CMD} -o ${XTGZ} ${XTGZURL} || \
				error_rm_wrkdir "could not retrieve ${XTGZURL}"
		else
			XTGZ=${OPTARG}
		fi
		;;
	*)
		usage
		error_rm_wrkdir
		;;
	esac
done

shift $(( OPTIND -1 ))
if (($# != 0)); then
	usage
	error_rm_wrkdir
fi

if [ -z "${SRCDIR}" -a -z "${TGZ}" -a -z "${XTGZ}" ]; then
	if [ -f "/usr/src/etc/Makefile" ]; then
		SRCDIR=/usr/src
	else
		error "please specify a valid path to src or (x)etcXX.tgz"
		usage
		error_rm_wrkdir
	fi
fi

[ -n "${SRCDIR}" -a ! -f "${SRCDIR}/etc/Makefile" ] && \
	error_rm_wrkdir "${SRCDIR} is not a valid path to src"

[ -n "${TGZ}" ] && ! tar tzf ${TGZ} ./var/db/sysmerge/etcsum >/dev/null 2>&1 && \
	error_rm_wrkdir "${TGZ} is not a valid etcXX.tgz set"

[ -n "${XTGZ}" ] && ! tar tzf ${XTGZ} ./var/db/sysmerge/xetcsum >/dev/null 2>&1 && \
	error_rm_wrkdir "${XTGZ} is not a valid xetcXX.tgz set"
	
TEMPROOT="${WRKDIR}/temproot"
BKPDIR="${WRKDIR}/backups"

do_populate
do_compare
do_post
