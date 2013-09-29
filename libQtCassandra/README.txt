
 * Thrift (communication between libQtCassandra and Cassandra)

The thrift-gencpp-cassandra folder is generated using thrift.

It is a good idea to regenerate the folder if your version is
different than what is offered in the libQtCassandra package.

The command line is:

  thrift -gen cpp cassandra.thrift

The cassandra.thrift file comes with Cassandra and is found in
the interface folder. (apache-cassandra-0.8.0/interface)

I had no problems with thrift 0.7.0 that I can remember. (It was
a bit difficult to find out all the headers and proper order to
include them, but no major problems.)

With version 0.8.0, I had to fix the CassandraProcessor interface
because it made use of apache::thrift::protocol instead of
::apache::thrift::protocol (:: missing at the start.) Also, the
netinet/in.h header was not included. I fixed that by including
that header in src/QCassandraPrivate.h before including thrift.
Note that for the two templates you'll need a space
(... boost::shared_ptr< ::apache::...> ...) since <: is viewed as
a special character by g++.

