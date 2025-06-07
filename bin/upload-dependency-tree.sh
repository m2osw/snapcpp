#!/bin/sh -e
#
# Send the dependency tree images to our snapwebsites.org server
# See: https://snapwebsites.org/project/references

if ! test -d BUILD
then
    echo "error: BUILD folder not found."
    echo "error: Are you running this script from the root snapcpp directory?"
    exit 1
fi

if ! test -f BUILD/Debug/clean-dependencies.svg
then
    echo "error: clean-dependencies.svg image not found."
    echo "error: Did you run make at least once?"
    exit 1
fi

scp BUILD/Debug/clean-dependencies.svg do.m2osw.com:/usr/drupal/sites/snapwebsites.org/files/images/dependencies.svg

# vim: ts=4 sw=4 et
