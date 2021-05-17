
# See API here
# https://launchpad.net/+apidoc/devel.html

from launchpadlib.launchpad import Launchpad
from os.path import expanduser

home = expanduser('~')
cachedir = home + '/.launchpadlib/cache/'
launchpad = Launchpad.login_anonymously('snapcpp', 'production', cachedir, version='devel')

