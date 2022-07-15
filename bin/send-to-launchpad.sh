#!/bin/sh -e

# Old distributions: xenial impish
DISTRIBUTIONS="bionic focal jammy"

while ! test -d BUILD/Debug -a -d BUILD/Release
do
    if test "`pwd`" = "/"
    then
        echo "BUILD folder not found"
        exit 1
    fi
    cd ..
done

if test "$1" = "-h" -o "$1" = "--help"
then
    echo
    echo "Usage: $0 [--distros \"<name> <name> ...\"] [project-name] [signature-email]"
    echo
    echo "  Where project-name defaults to \"snapwebsites\""
    echo "  and it can be set to any one of our contrib as well"
    echo
    echo "Valid project-names are:"
    echo
    TERMINAL_WIDTH=`tput cols`
    ls contrib/ | \
        tr '[:upper:]' '[:lower:]' | \
        grep -v cmakelists.txt | \
        sed -e '$a\ \ snapcmakemodules' -e '$a\ \ snapwebsites' -e 's/^/  /' | \
        sort | \
        pr --omit-pagination --omit-header \
            --width=${TERMINAL_WIDTH} --page-width=${TERMINAL_WIDTH} \
            --indent=0 --column=5
    echo
    exit 1;
fi

if test "$1" = "--distros"
then
    shift
    DISTRIBUTIONS="$1"
    shift
fi

if test -n "$1"
then
    MODULE="$1"
else
    MODULE=snapwebsites
fi

if test -n "$2"
then
    DEBEMAIL="$2"
else
    if test -f ~/.build-snap.rc
    then
        . ~/.build-snap.rc
    fi
    if test -n "$DEBUILD_EMAIL"
    then
        DEBEMAIL="$DEBUILD_EMAIL"
    else
        DEBEMAIL="Build Server <build@m2osw.com>"
    fi
fi

TMP=../tmp

# Generating the source must be done in the concerned folder
case $MODULE in
"snapwebsites")
    cd snapwebsites
    ;;
"cmake"|"snapcmakemodules")
    cd cmake
    MODULE=snapcmakemodules
    ;;
"snapbuilder")
    echo "error: snapbuild is not to be sent to launchpad."
    exit 1
    ;;
*)
    cd contrib/$MODULE
    TMP=../../tmp
    ;;
esac

# Verify that version is 4 numbers separated by 3 periods
# The "~<name>" part was removed, we should not ever need it
#
VERSION=`dpkg-parsechangelog -S version`
VALID=`echo "$VERSION" | sed -e 's/[0-9]\+\.[0-9]\+\.[0-9]\+\.[0-9]\+~[a-z]\+//'`
if test -n "$VALID"
then
    echo "error: invalid version \"$VERSION\": must be 4 numbers separated by periods and followed by a distribution name."
    exit 1
fi

# Remove the distro from the version
VERSION_NUMBER=`echo "$VERSION" | sed -e 's/~[a-z]\+//'`

SOURCE_DIR=`pwd`

if ! git diff-index --quiet HEAD --
then
    git status
    echo "***"
    echo "*** error: you have uncommited changes."
    echo "***"
    echo "*** error: this script makes changes to the debian/changelog"
    echo "*** error: and debian/control files and we don't want you to lose"
    echo "*** error: any data; please cd to ${SOURCE_DIR} and git commit"
    echo "*** error: and then try again. Thank you."
    echo "***"
    exit 1
fi

trap 'restore_files' EXIT

restore_files() {
    # Restore the files we just changed so it doesn't look like we need a new
    # commit and that a different distro is selected
    #
    (
        cd ${SOURCE_DIR}/debian
        git checkout changelog control
    )
}

for distro in ${DISTRIBUTIONS}
do
    # Before we generate the output, we need to fix the distribution name
    # in each version found in the control file and the first line of the
    # changelog

    sed -i -e "s/\([0-9]\+\.[0-9]\+\.[0-9]\+\.[0-9]\+\)~[a-z]\+/\1~${distro}/g" debian/control
    sed -i -e "1 s/\([0-9]\+\.[0-9]\+\.[0-9]\+\.[0-9]\+\)~[a-z]\+/\1~${distro}/" \
            -e "1 s/ [a-z]\+;/ ${distro};/" debian/changelog

    debuild -S -d -sa -m"${DEBEMAIL}"

    # To send the source to Launchpad, we need to be at the same level as those
    # files
    cd ..

    dput ppa:snapcpp/ppa ${MODULE}_${VERSION_NUMBER}~${distro}_source.changes

    # Get rid of those files from our source tree
    mkdir -p $TMP/sources
    mv ${MODULE}_${VERSION_NUMBER}* $TMP/sources/.

    rm -f ${SOURCE_DIR}/debian/files

    cd ${SOURCE_DIR}
done


# vim: ts=4 sw=4 et
