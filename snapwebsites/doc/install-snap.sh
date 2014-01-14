#!/bin/sh
#
# Run with sudo as in:
#   sudo sh snap.sh
#

##
## Apache2 installation
##
# Apache should only be installed on front end computers
if ! test -f /etc/apache2
then
	apt-get install apache2
fi

##
## Cassandra sources (no PPA at this point)
##
add-apt-repository 'deb http://www.apache.org/dist/cassandra/debian 20x main'
# Ad this point the source generates an error in 13.10
#add-apt-repository 'deb-src http://www.apache.org/dist/cassandra/debian 20x main'
gpg --keyserver pgp.mit.edu --recv-keys F758CE318D77295D
gpg --export --armor F758CE318D77295D | sudo apt-key add -
gpg --keyserver pgp.mit.edu --recv-keys 2B5C1B00
gpg --export --armor 2B5C1B00 | sudo apt-key add -

##
## Snap! CMS PPA
##
add-apt-repository ppa:snapcpp/ppa

##
## Update apt-get database
##
# Since we added new sources we have to run a full update first
apt-get update


##
## Install Cassandra and Snap! CMS
##
# Now install all the elements we want to get
apt-get install cassandra snapserver snapcgi

# Automatically installed if not yet available:
#
# Standard libraries and tools
#
#  . liblog4cplus
#  . libqt4-bus
#  . libqt4-network
#  . libqt4-script
#  . libqt4-xml
#  . libqt4-xmlpatterns
#  . libqtcore4
#  . qdbus
#  . qtchooser
#
# Snap! CMS libraries and tools
#
#  . libqtcassandra
#  . libqtserialization
#  . libsnapwebsites
#  . libthrift0
#  . libtld
#  . snapcgi
#  . snapwebserver
#  . snapserver-core-plugins


