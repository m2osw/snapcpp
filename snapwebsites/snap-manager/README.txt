
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
you should have a websites 'www.domain.com' entry. The list command can
be used to list all the entries at once.

When running Snap! Manager, click on the Connect button at the bottom left
(In the Cassandra area.)

Click on the Domains tab which should have been highlited (if not, the
Cassandra connection did not work as expected.)

In the Domain Name box enter the name of a domain (i.e. "example.com"). Do
not include sub-doman names here. It is forbidden. The user can only
specificy a domain. In the Rules box, enter a valid rule such as:

main {
  required host = "www\.";
};

At that point, click on Save. The new entry appears in the left column.

Clicking on that entry and you see the Websites tab appear. That tab allows
you to edit the different websites supported by that one domain.

The Websites definitions are very similar to the Domains definitions. You
have to enter the full sub-domain + domain name at the top. To continue with
our present example, it would be www.example.com (i.e. the domain was
"example.com" andthe rules defined "www\." as the host name.) Once you entered
a valid name, youc an enter a rule such as the following:

main {
  protocol = "http";
  port = "80";
};

That should get you going as it is enough to get started. Just make sure
to replace the "www" with the sub-domain that you assigned to your domain.

Details can be found on the Help page:

  http://snapwebsites.org/help/snap-manager



Testing on Apache

Start Server with

   ../BUILD/snapwebsites/src/snapserver -c src/snapserver.conf -d

   (you may want to edit the snapserver.conf file for your system...)

Then go to those pages (replace the domain name with yours):

   http://csnap.snapwebsites.com/cgi-bin/snap.cgi?q=/admin
   http://csnap.snapwebsites.com/cgi-bin/snap.cgi?q=/robots.txt

If Apache is properly setup, then the CGI binary does not need to be
specified:

   http://csnap.snapwebsites.com/admin
   http://csnap.snapwebsites.com/robots.txt

The following are the Apache rules I use for that purpose. Note that it is
not yet solid enough for a final version of your server, but it works good
enough.

   # See http://httpd.apache.org/docs/current/mod/mod_rewrite.html
   RewriteEngine on
   RewriteRule snap\.cgi - [S=1]
   RewriteRule ^(.*)$ /cgi-bin/snap.cgi?q=/$1 [L,PT,QSA,E=CLEAN_SNAP_URL:1]

