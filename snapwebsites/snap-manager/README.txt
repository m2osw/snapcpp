
The following are quick instructions to check the data saved in
your Cassandra cluster:

Start the CLI:

   cassandra-cli

At the Cassandra prompt:

   connect localhost/9160;

   use snap_websites;

   list domains;

   list websites;

   get domains[utf8('domain.com')];

   get websites[utf8('www.domain.com')];

   list sites;

   del sites[ascii('http://you.site.key/')][ascii('core::last_updated')];

   list content;

   list links;

The 'domain.com' is whatever domains you entered with Snap Manager.

The websites entries are more complex since they have to match a domain
name first, so assuming 'domain.com' accepts 'www' as a sub-domain name,
you should have a 'www.domain.com' entry. The list command can be used
to list all the entries at once.

Testing on Apache

Start Server with

   ../BUILD/snapwebsites/src/snapserver -c src/snapserver.conf -d

Then go to those pages

   http://alexis.m2osw.com/cgi-bin/snap.cgi?q=/admin
   http://alexis.m2osw.com/cgi-bin/snap.cgi?q=/robots.txt

