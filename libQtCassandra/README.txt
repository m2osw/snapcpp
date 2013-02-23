
 * Updates

   - 0.4.7 added support for a Cassandra-only Lock mechanism

     o Added many missing updates to this README.txt file.
     o Fixed some documentation.
     o Added support for specifying the consistency on a value
       read by setting the consistency in the cell.
     o Added a synchronization function so we can wait and have all the
       nodes running with the same schema version.
     o Fixed the QCassandra::Snitch() so it returns the snitch, not the
       protocol version.
     o Added a test to verify that locks work when processes run on any
       number of clients.
     o Updated the tests to properly work in a cluster environment.
     o Added a unit test for the QCassandraValue class which checks
       a large number of cases, this allowed me to apply fixes.
     o The <, <=, >, >= QCassandraValue operators were invalid.
     o Added Bool support in the QCassandraValue object.
     o Added an index to the QCassandraValue data; and a size
       for strings and binaries (although a UTF-8 size...)
     o Moved the read of values to global functions so they are
       accessible from other functions without having to use
       QCassandraValue variables
     o Fixed the findContext() so it loads the Cassandra contexts first
       if not yet loaded.
     o Fixed the disconnect() function so it clears all the variables
       and the same Cassandra object can safely be reused.
     o Fixed the QCassandraPrivate::getColumnSlice() and
       QCassandraPrivate::getRowSlices() functions which would not
       first transform the column key in a QByteArray and thus binary
       keys with null characters would invariably fail.
     o Fixed the QCassandraRow::dropCell() as it would emit a read of
       the cell before actually dropping it. The read would retrive the
       consistency value and/or the timestamp. These values are retrieved
       from memory only now, if defined.
     o Fixed the CMakeLists.txt so the thrift library is linked against
       the libQtCassandra library itself; that way it is not required in
       the binaries that link against the libQtCassandra library.
     o Removed all references to the boost_system library.
     o Reviewed the SSL connection capability. It is still not considered
       to be working but the password can now be specified from your
       application.
     o Included 2013 in copyright notices.

   - 0.4.6 added support for UUID and char */wchar_t *

     o Added direct support for QUuid as row and column keys.
     o Added direct support for char * and wchar_t * so we do not
       have to first cast strings to QString everywhere.
     o Fixed bug testing row key size to limit of 64535 instead
       of 65535.
     o Added a test as row and column keys cannot be empty. It will
       now throw an error immediately if so.
     o Updated some documentation accordingly and with enhancements.

   - 0.4.5 add support to read columns used as indexes

     o Added a first_char and last_char variables (QChar) in column predicate
       which can be used to define "[nearly] All column names".
     o Fixed the names of two functions: setFinishColumnName() and
       setFinishColumnKey() are now setEndColumnName() and setEndColumnKey()
       respectively (as documented and so it matches the getters.)
     o Added support for indexes defined with columns. The column predicate
       now has a setIndex() function and that allows you to call readCells()
       repititively until all the columns matching the predicate were returned
       (very similar to reading a large set of rows.)
     o Fixed a few things in the documentation.

   - 0.4.4 add some composite column support

     o Added support for composite columns. It was functional before but
       with knowledge on how to build the column key which is actually
       quite complicated (okay, not that hard, but libQtCassandra is here
       to hide that sort of thing!) Use the compositeCell() function of
       your QCassandraRow objects.

   - 0.4.3 added support for counters

     o Added support for counters.
     o Fixed several usage of keys so 0 bytes works as expected
       (in getValue() and insertValue().)
     o Small fixes to documentation.

   - 0.4.2 primarily fixes the problem with readRows() which could not
           be used to read more than count rows because the following
	   call would return an invalid set of entries.

     o Fixed readRows()
     o Fixed replicateOnWrite()
     o Fixed CMakeLists.txt in regard to thrift
     o Fixed prepareContextDefinition()
     o Added support for 1.0 and 1.1
     o Added the million_rows stress test
     o Upgraded to thrift 0.8.0

   - 0.4.1 many fixes to basic data

     o Fixed the size of the buffer used to save 64 bit integers.
     o Fixed the size of integers used to handle floating points.
     o Fixed the double being read as 8 bytes and somehow converted to a
       float instead of a double.
     o Fixed the test of the string set in a value to limit the UTF-8 version
       of the string to 64Mb (instead of the number of UCS-2 characters held
       by a QString.)
     o Enhanced documentation about the findRow() and findCell() which do not
       look for a row or cell in the Cassandra system, it only checks in memory!
     o Better support older versions of g++ (4.1 cannot properly cast the
       controlled variables for enumerations) -- thank you to John Griswold
       for reporting the problem.
     o Added some missing documentation.

   - 0.4.0 package clean up and enhanced cmake scripts

     o Enhanced the cmake scripts to make it even easier (find/use Qt, Thrift)
       and thus I jumped to version 0.4.0 because this is a pretty major change
       from 0.3.x
     o Removed the Qt sub-folder names from \#include.
     o Made the getValue() function return false so we can know when it fails
       and react accordingly.
     o Fixed the use of the slice predicate and ignore the strings null
       terminator as they ought to be (i.e. a key can include a nul character.)
     o Added some try/catch to avoid a certain number of fairly legal
       exceptions (i.e. missing value or column.)
     o Removed all unwanted files from source package using a CPACK option.
     o Strip folder name from documentation to make it smaller.
     o Updated all copyrights to include 2012.

   - 0.3.2 fixed column predicate

     o Fixed the creation of a row predicate as it wasn't defining a column
       predicate which is necessary when we call readRows() with the default
       parameters on a table.

   - 0.3.1 ameliorated cmake scripts

     o Added support for installation targets and generation of binary packages.

   - 0.3.0

     o Added a dropContext() in the QCassandra object.
     o Added proper unparenting of the context and table classes.
     o Started to make use of the Controlled Variables (requires 1.3.0
       or better.)


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

