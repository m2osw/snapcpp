
To make modifications, it is preferable to add the changes to files under
this folder. The name must be a two digit number (00 to 99) followed by
a dash and the name of the configuration file. For example, the
`sitter.conf` would be found here as:

    /etc/sitter/sitter.d/50-sitter.conf

This allows for changes to the default .conf file to happen under the hood
and your changes to remain in place at all time.

**Note:** it is prefered that you use 50-... because other tools and
libraries may add their own configuration files that they want applied
before (<50) or after (>50) your own configuration.

