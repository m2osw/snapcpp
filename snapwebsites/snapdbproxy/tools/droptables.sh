#!/bin/sh
#
# WARNING: This script is VERY SLOW. Deleting all the tables is slower that
#          recreating the database from scratch (i.e. stop Cassandra, delete
#          all the data under /var/lib/cassandra/* and then resstarting
#          Cassandra); however, it keeps the "domains" and "websites" tables
#          around meaing that you do not need to use snapmanager to define
#          those entries again
#

# Make sure that snapdb was installed
if ! hash snapdb
then
    echo "error: could not find \"snapdb\" on this computer. Was it installed?"
    exit 1
fi

HOST="$1"
shift

if test -z "$HOST"
then
    echo "error: you must specific the host that snapdb has to use to access the database (i.e. 127.0.0.1)"
    exit 1
fi

echo "Reset all the tables so you can try to reinstall a website."
echo
echo "WARNING: This command ***DESTROYS ALL YOUR DATA***"
echo "WARNING: Normally, only programmers want to run this command."
echo "WARNING: Please say that you understand by entering:"
echo "WARNING:     I know what I'm doing"
echo "WARNING: and then press enter."
echo
read -p "So? " answer

if test "$answer" != "I know what I'm doing"
then
    echo "error: the database destruction was canceled."
    exit 1
fi

snapdb --host "$HOST" --drop-table --yes-i-know-what-im-doing antihammering
snapdb --host "$HOST" --drop-table --yes-i-know-what-im-doing backend
snapdb --host "$HOST" --drop-table --yes-i-know-what-im-doing branch
snapdb --host "$HOST" --drop-table --yes-i-know-what-im-doing cache
snapdb --host "$HOST" --drop-table --yes-i-know-what-im-doing content
#snapdb --host "$HOST" --drop-table --yes-i-know-what-im-doing domains
snapdb --host "$HOST" --drop-table --yes-i-know-what-im-doing emails
snapdb --host "$HOST" --drop-table --yes-i-know-what-im-doing epayment_paypal
snapdb --host "$HOST" --drop-table --yes-i-know-what-im-doing epayment_stripe
snapdb --host "$HOST" --drop-table --yes-i-know-what-im-doing files
snapdb --host "$HOST" --drop-table --yes-i-know-what-im-doing firewall
snapdb --host "$HOST" --drop-table --yes-i-know-what-im-doing layout
snapdb --host "$HOST" --drop-table --yes-i-know-what-im-doing links
snapdb --host "$HOST" --drop-table --yes-i-know-what-im-doing list
snapdb --host "$HOST" --drop-table --yes-i-know-what-im-doing listref
snapdb --host "$HOST" --drop-table --yes-i-know-what-im-doing mailinglist
snapdb --host "$HOST" --drop-table --yes-i-know-what-im-doing password
snapdb --host "$HOST" --drop-table --yes-i-know-what-im-doing processing
snapdb --host "$HOST" --drop-table --yes-i-know-what-im-doing revision
snapdb --host "$HOST" --drop-table --yes-i-know-what-im-doing secret
snapdb --host "$HOST" --drop-table --yes-i-know-what-im-doing serverstats
snapdb --host "$HOST" --drop-table --yes-i-know-what-im-doing sessions
snapdb --host "$HOST" --drop-table --yes-i-know-what-im-doing shorturl
snapdb --host "$HOST" --drop-table --yes-i-know-what-im-doing sites
snapdb --host "$HOST" --drop-table --yes-i-know-what-im-doing test_results
snapdb --host "$HOST" --drop-table --yes-i-know-what-im-doing tracker
snapdb --host "$HOST" --drop-table --yes-i-know-what-im-doing users
#snapdb --host "$HOST" --drop-table --yes-i-know-what-im-doing websites

echo "Done."
echo
echo "Now you may recreate the table, empty, with the command:"
echo "         snapcreatetables"
echo
echo "Then later reinstall the websites with:"
echo "         snapinstallweb --domain <your-[sub-]domain> --port <80|443>"
echo

# vim: ts=4 sw=4 et
