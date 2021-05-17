#!/bin/sh
#
# This script retrieves the launchpadlib which we use to access the launchpad
# API to see where a build is at (especially whether it's still building or
# not--if not we can then check whether it worked and move forward to the
# next package if so)
#
# Commands come from:
#   https://help.launchpad.net/API/launchpadlib

mkdir -p tmp
cd tmp

echo
echo "========================= lp:oauth ========================="
echo

bzr branch lp:oauth
cd oauth
chmod 755 ./setup.py
sudo ./setup.py install
cd ..

echo
echo "========================= lp:lazr.uri ========================="
echo

bzr branch lp:lazr.uri
cd lazr.uri
chmod 755 ./setup.py
sudo ./setup.py install
cd ..

echo
echo "========================= lp:lazr.restfulclient ========================="
echo

bzr branch lp:lazr.restfulclient
cd lazr.restfulclient
chmod 755 ./setup.py
sudo ./setup.py install
cd ..

echo
echo "========================= lp:wadllib ========================="
echo

bzr branch lp:wadllib
cd wadllib
chmod 755 ./setup.py
sudo ./setup.py install
cd ..

echo
echo "========================= lp:launchpadlib ========================="
echo

bzr branch lp:launchpadlib
cd launchpadlib
chmod 755 ./setup.py
sudo ./setup.py install
cd ..

