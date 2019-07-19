#!/bin/sh -e
#
# Send all our references to our snapwebsites.org server
# See: https://snapwebsites.org/project/references

if ! test -d ../BUILD
then
    echo "error: BUILD folder not found."
    echo "error: Are you running this script from the root snapcpp directory?"
    exit 1
fi

scp ../BUILD/contrib/*/doc/*.tar.gz ../BUILD/snapwebsites/*/doc/*.tar.gz do.m2osw.com:/usr/drupal/sites/snapwebsites.org/files/references/.

# vim: ts=4 sw=4 et
