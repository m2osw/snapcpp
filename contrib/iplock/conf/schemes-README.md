
Administrator Modified and Additional Scheme Files
==================================================

Please copy files from the `/etc/iplock/schemes` directory to
this `/etc/iplock/schemes/schemes.d` sub-directory and modify
them here. Or create new scheme files as required by your
systems.

That way, you will continue to get the default configuration
changes from the source package under `/etc/iplock/schemes`.

All files get first loaded from `/etc/iplock/schemes` and then
again from `/etc/iplock/schemes/schemes.d`. Any parameter redefined
in the sub-directory overwrites the parameter of the same name in
the main directory.


Parameters in a Scheme File
===========================

A scheme file is currently composed of 4 entries as follow:

    ports=
    check=
    block=
    unblock=

The `ports` parameter defines a list or ports to block whenever this
scheme is specified. Each port number is separated by a comma.

The `check` parameter defines a command line the process can use
to check whether a rule exists or not. If the rule exists, the
process is expected to exit with 0. If the rule cannot be found,
the process is expected to exit with 1.

The `check` command is run whether you are try to `--block` or
`--unblock` an IP address. When blocking, nothing else happens if
the IP is already in the chain list. When unblocking, nothing else
happens if the IP is not in the chain list.

The `block` command is used to add the IP address to the iptables
chain. The add can use an _append_ (`-A`) or an _insert_ (`-I`)
command. We prefer the insert to get the IP address at the start
of the chain, since this is a hot request, it will be quicker for
the ipfilter system to block the IP if found earlier.

The `unblock` command is used to remove the IP address from the
iptables chain. The remove uses the _delete_ (`-D`) command.

For additional details, check out the http.conf and smtp.conf files.

