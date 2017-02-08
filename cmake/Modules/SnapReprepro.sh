#!/bin/bash
#
# This script runs the "reprepro" binary against the pbuidler dir.
# `*.changes` files are expected to exist. You must pass in a component
# name (e.g. contrib, non-free, main, etc.).
#

# Handle command line
#
set -e
if [ -z "$1" ] && [ -z "$2" ] && [ -z "$3" ]
then
    echo "usage: $0 distribution component destpath"
    exit 1
fi

DIST=$1
COMP=$2
DEST=$3

reprepro -C ${COMP} -Vb ${DEST} removematched ${DIST} "*"

INDIR=${HOME}/pbuilder/${DIST}_result/${COMP}
for file in ${INDIR}/*.deb
do
    reprepro -C ${COMP} -Vb ${DEST} includedeb ${DIST} ${file}
done

# vim: ts=4 sw=4 et
