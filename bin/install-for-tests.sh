#!/bin/sh -e
#
# Install additional parameters that make the tests run a more complete
# coverage.
#

echo "--- making sure user snapwebsites exists"
if ! getent passwd snapwebsites >/dev/null
then
	echo "--- creating missing user, snapwebsites"
	sudo useradd --shell /sbin/nologin snapwebsites
fi

echo "--- making sure group snapwebsites exists"
if ! getent group snapwebsites >/dev/null
then
	echo "--- creating missing group, snapwebsites"
	sudo groupadd snapwebsites
fi

if ! getent group adm | sed -e 's/.*://' -e 's/,/ /' | grep "\<${USER}\>" > /dev/null
then
	sudo groupmod snapwebsites --append --users "${USER}"
fi

if ! grep lcov /etc/fstab
then
	# This requires the folder to be shared in the domain
	# definitions which cannot be done here...
	#
	# See m2osw-server/create-build-vm.sh (non-public) or
	# https://linux.m2osw.com/qemu-run-vms
	#
	sudo echo "# shared folder" >> /etc/fstab
	sudo echo "lcov /mnt/lcov virtiofs rw 0 0" >> /etc/fstab
fi
