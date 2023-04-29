#!/bin/sh -e
#

FULL_REPORT=false
LATEST=false
while test -n "$1"
do
	case "$1" in
	"--help"|"-h")
		echo "Usage: $0 [--opts]"
		echo "where --opts is one or more of the following:"
		echo "  --help | -h      print out this help screen"
		echo "  --latest | -l    pull the latest"
		exit 1
		;;

	"--full-report"|"-f")
		shift
		FULL_REPORT=true
		;;

	"--latest"|"-l")
		shift
		LATEST=true
		;;

	*)
		echo "error: unknown option \"$1\"."
		exit 1
		;;

	esac
done

TMPDIR=`pwd`/tmp
CONTRIB=contrib
if ! test -d ${CONTRIB}
then
	if test -d ../contrib
	then
		CONTRIB=../contrib
		TMPDIR=`pwd`/../tmp
	elif test -d ../../contrib
	then
		CONTRIB=../../contrib
		TMPDIR=`pwd`/../../tmp
	else
		echo "error: could not find the contrib/... directory."
		exit 1
	fi
fi

trap 'rm -f status.txt checkout.txt error.txt pull.txt' EXIT

mkdir -p "${TMPDIR}"

for f in ${CONTRIB}/*
do
	# The dev/coverage of zipios still creates this folder...
	#
	if test "${f}" = "BUILD"
	then
		continue
	fi

	if test -d ${f}/debian
	then
		(
		up_to_date=false
		checkout=true
		pull=true

		cd ${f}
		echo "`pwd`\033[K\r\c"
		git status . >${TMPDIR}/status.txt
		report=`cat ${TMPDIR}/status.txt | tr '\n' ' '`
		if test "${report}" = "On branch main Your branch is up to date with 'origin/main'.  nothing to commit, working tree clean "
		then
			up_to_date=true
		fi

		if ${LATEST}
		then
			git checkout main >${TMPDIR}/checkout.txt 2>${TMPDIR}/error.txt
			report=`cat ${TMPDIR}/checkout.txt`
			if test "${report}" = "Your branch is up to date with 'origin/main'."
			then
				checkout=true
			else
				checkout=false
			fi

			git pull >${TMPDIR}/pull.txt
			report=`cat ${TMPDIR}/pull.txt`
			if test "${report}" = "Already up to date."
			then
				pull=true
			else
				pull=false
			fi
		fi

		if ${FULL_REPORT} || ! ${up_to_date} || ! ${checkout} || ! ${pull}
		then
			echo "----------------\033[K"
			pwd

			echo
			if ${FULL_REPORT}
			then
				echo "  Full Report\c"
			fi
			if ! ${up_to_date}
			then
				echo "  Not Up to Date\c"
			fi
			if ! ${checkout}
			then
				echo "  Not Up to Date\c"
			fi
			if ! ${pull}
			then
				echo "  Not Latest Pull\c"
			fi

			echo
			echo "$ git status ."
			cat ${TMPDIR}/status.txt

			echo
			echo "$ git checkout main"
			cat ${TMPDIR}/error.txt
			cat ${TMPDIR}/checkout.txt

			echo
			echo "$ git pull"
			cat ${TMPDIR}/pull.txt

			echo
		fi
		)
	fi
done


