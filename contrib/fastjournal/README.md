
<p align="center">
<img alt="advgetopt" title="A Fast Journal Database."
src="https://snapwebsites.org/sites/snapwebsites.org/files/images/fastjournal.svg" width="277" height="277"/>
</p>

# Introduction

The libfastjournal library, deamon, and client are here to handle fast write
and read in a ring-like buffer accross a network.

# Reasons Behind Having Our Own Library

Most of the tools I've seen out there are either written in Java or make use
or thousands of features which bloat such a functionality. We want a
minimalistic implementation which is very efficient.

# Requirements For Our Journals

## A Simple FIFO of Jobs

In many cases, a journal can be imlemented with a simple FIFO. Whenever you
detect that a job is required, you push it on the FIFO.

    Time   Frontend      Backend

    1      Do Job 1
    2                    Work on Job 1
    3
    4
    5      Do Job 2
    6
    7
    8                    Work on Job 2
    9
    10    Do Job 3
    11                   Work on Job 3
     ...

In this example, Job 1 takes a long time so Job 2 sits in the journal for
a few cycles. Job 2, though, is very fast and thus the backend starts work
on Job 3 as soon as it gets added.

So when the backend managing the FIFO is fast enough to handle all the jobs
faster than they are added (at least within a given window of time,) then
you are fine with a simple FIFO.

However, 99% of the time, this is not the case. Or worse, it is a huge waste
of time because you'd end up running the same processes over and over again
when you could limit to one process per resource.

## Our More Advanced FIFO

In our case we need a few additional features to make sure that things
work as expected:

* Priority -- setup a priority over what gets worked on next; this is
  important when some processes should be run immediately (ASAP at least)
  opposed to processes that can wait.

* No Duplicates -- we want to avoid having two requests for the same jobs;
  in our case, it happens a lot that we update one value in a row and then
  another and yet another and each time we send the row as being in need of
  updating to the backend; that means we'd have to run the exact same backend
  job three times in a row when one time is really enough because the second
  and third time will give the exact same results.

* Not Immediate -- when we make changes, we want the backend to wait for the
  frontend to be done; the frontend may make changes to many different rows
  and that can have side effects to the rows we want updated (i.e. a list
  points to row X, Y, Z and the sort of that list depend on the value of
  X, Y and Z...); for that reason we want to have a timestamp in the future
  for when the work as to be started.

## Snap Requirements for Lists

The lists in the Snap! environment are computed each time a row is updated.
What happens is that a list checks pages that get updated and for inclusion.
The list can then be updated to include or exclude the pages that were
updated. This works well on websites such as blogs where the number of
list is limited and there is no support for advance indexes in your database
(like in Cassandra where it's at least somewhat limited).

As a result, our requirements are defined like so:

* Priority -- each item in the FIFO are prioritized, this allows us to run
  more important jobs first (one byte, higher priorities are represented
  by smaller numbers).

* Timestamp -- within the same priority, we want to still handle jobs as if
  they were added in a FIFO and not dependent to any other sorting parameter;
  to do so we use a timestamp of when the item needs to be worked on
  (i.e. we often want to give a little delay between the change happening on
  the front end and the updates done by the backend).

* Resource -- the resource that we need to work on; this can be all sorts of
  things, in our Snap! environment it is just a URI but you could include a
  function name or ID which tells you what needs to be done to that URI;
  in our case, though, we want to be able to extract the domain name from
  this resource URI

In our existing implementations, the jobs never expire so a TTL is not
currently useful.

## General Sort Index

The sort is done using a key which looks like this:

    <domain(resource)>:<priority>:<timestamp>

Note that means we first check the domain and priority parameters, but our
`GET` needs to be able to ignore keys that have `<timestamp>` set in the
future. As a result, this means the indexes with these entries have "gaps".
For example, the following two rows are sorted properly:

    example.com:1:<now + 10 min.>
    example.com:2:<now - 3 min.>

**NOTE:** The next processing time is also using the timeout of the worked
on rows. When a row is being worked on and it's marked as taking 2h (For
example), then that row is hidden until the work on it is done.

### Dynamic Index on `timestamp`

But with such an index the `GET` has to skip the first entry.

This limit is due to the fact that indexes in a database is not dynamic
by default. Another method is to use the timestamp as a separate index:

    <timestamp>

First go through the `<timestamp>` index and discover the rows that can
make it to the General Sort Index. That way all the entries in the
General Sort Index are ready to be processed. We still keep the
`<timestamp>` field in that other index because we still want that FIFO
order.

However, having the General Sort Index with all the rows allows us to find
the next time and date for a given domain and have a specific wait for the
next possible processing to take place.

### URI Index

The Journal is used to request for a job done on a given URI. The job is
always going to be the same. That means we want to have unicity (at least
per job although our current version only supports one job--TBD).

For that reason we have an index to find existing entries so we can update
them instead of doing the job multiple times in a row.

    <uri>

Note that in the definition I have in MySQL, the journal table includes an
identifier. This `<uri>` index references the row using that identifier.



The frontend and backend must both be safe. That is, the frontend wants
to save the requests to disk, send them to the backend, remove them from
disk only once the backend acknowledge receipt. Similarly, the backend
has to be write the value to disk before we move forward.

Here is our existing MySQL table. The problem with MySQL is that it tends
to grow a lot in terms of memory usage.

    CREATE TABLE snaplist.journal
      ( id              INT NOT NULL PRIMARY KEY AUTO_INCREMENT
      , domain          TEXT NOT NULL          -- will change to the domain ID once SNAP-381 is done
      , priority        SMALLINT NOT NULL      -- WARNING: don't use TINYINT because this is a number from 0 to 255 and TINYINT is -128 to +127
      , key_start_date  BIGINT NOT NULL        -- Unix timestamp in microseconds
      , uri             TEXT NOT NULL          -- will change to the page ID once SNAP-381 is done
      , status          BIGINT DEFAULT NULL    -- new if NULL, working if set, if set for more than 24h, consider work failed, Unix timestamp in microseconds
      , next_processing BIGINT AS (IF(status, status, key_start_date)) NOT NULL
                                               -- this is the timestamp when we next want to process this row
      , INDEX journal_index USING BTREE (domain(64), priority, key_start_date) COMMENT 'used to read the next page(s) to work on'
      , INDEX journal_uri USING BTREE (uri(256)) COMMENT 'used to delete duplicates (we keep only the latest entry for each URI)'
      , INDEX journal_next_row USING BTREE (domain(64), next_processing) COMMENT 'used to determine when to wake up next'
      )

# Implementation

The system includes several parts:

1. A daemon which runs on a backend server; that deamon is what manages the
   ring buffer for you; it uses memory for the data, but always saves the
   data to disk to make sure it is available in case of a crash or a restart.

2. Daemons are capable (future version, though) of communicating between
   each others so we decrease the chances of data loss by having duplicates
   on multiple computers.

3. On the client side, we also have a local journal used to save the requests
   locally until the backend replies with a positive answer that the data was
   indeed received by the server; that way we can be sure that the possibility
   of data losses are very minimal.

4. All accesses are done using the library.

## Modules part of the Project

The project is composed of several modules as we can see here. The fact is
that we work with many parts which make sure that everything is as fast
as possible.

* Module 1 -- client sending data, gets written to file which is
  extermely fast

    The client that wants some processing done just links to the Fast
    Journal library, creates a class, setups the fields, and then saves
    that data to a file. Most of the work is done in the Fast Journal library.
    The write is an append so we do not even need to lock the file. The
    reader reads the specified size. If not enough data is found, it can
    _sleep_ and try again until all the data was read.

        +----------+       +------------------------+
        |          | Link  |                        |
        |  Client  +------>|  Fast Journal Library  |
        |          |       |                        |
        +----------+       +------------------------+

    We do not _sleep_ to wait for the data to be available. Instead we
    have a listener which can be used with a `poll()` so we wake up as
    soon as more data is available.

* Module 2 -- fastjournal-client reads file and send to backend

    Along the Client, we have a Fast Journal Client service which listens
    to changes of the local file. When changes do happen, it forwards
    the information to the backend: Fast Journal Daemon.

        +------------------------+     +------------------------+
        |                        | TCP |                        |
        |  Fast Journal Client   +---->|  Fast Journal Daemon   |
        |                        |     |                        |
        +-----------+------------+     +----------+-------------+
                    | Link                        | Link
                    v                             v
        +------------------------+     +------------------------+
        |                        |     |                        |
        |  Fast Journal Library  |     |  Fast Journal Library  |
        |                        |     |                        |
        +------------------------+     +------------------------+

    The Fast Journal Client uses a Lock to make sure that it only works
    on a file when the Client is not working on it. Since it reads one
    line at a time, it is really fast. Once done with a line (i.e. it
    had confirmation that it was sent and properly persisted by the
    Fast Journal Daemon) then it erases the line on the client.

    If you restart the Fast Journal Client, it will skip empty lines
    so it doesn't repeat lines that were successfully sent to the
    Fast Journal Daemon.

    In my current implementation, this journal is created for 1h. We
    only append data and after 1h we work on the next file. The Fast
    Journal Client deletes the files it is done with so next time you
    write to one of those files, it will be non-existant so an append
    will create the file and write a first line.

    If the Fast Journal CLient doesn't run for a while, the data continues
    to cumulate, but it doesn't get lost.

    The client sends a PING to the Fast Journal Client to wake it up after
    a write. This makes sure that writes can be handled as quickly as
    possible.

* Module 3 -- read from fastjournal-daemon

    A Backend Consumer connects to the Fast Journal Daemon
    directly. The consumer then requests the next URI that
    needs to be worked on.

    Again, the library takes care of most of the work. Here we
    want the backend to mark the journal entry as being worked
    on. It is done that way in case the backend fails and thus
    the work is not complete. Some Backends may want to just
    delete the batch request immediately anyway.

        +------------------------+     +------------------------+
        |                        | TCP |                        |
        |  Backend Consumer      |<----+  Fast Journal Daemon   |
        |  (batch processor)     |     |   +-> memory           |
        |                        |     |       & read from file |
        +-----------+------------+     +----------+-------------+
                    | Link                        | Link
                    v                             v
        +------------------------+     +------------------------+
        |                        |     |                        |
        |  Fast Journal Library  |     |  Fast Journal Library  |
        |                        |     |                        |
        +------------------------+     +------------------------+

    The Backend Consumer connects and sends a "NEXT" command.

    The Fast Journal Daemon checks its data to determine whether
    there is a batch to work on, if so it returns that, otherwise
    it returns either "nothing to work on" or "next job happens
    at _timestamp_".


## Key/Value Pairs Management

On the server (and client may also have some copy of the data), we want to
have a map of all the values currently kept in memory. That map is also
reflected on disk where it may be much bigger. When the map reaches a certain
size, we can look at reducing the in memory map by removing the values that
have not been touched in a while. For that reason we keep a "last accessed"
timestamp in each structure.

The data we save is as follow:

    keys -- the keys used to access the value
    type -- the type of the value
    value -- the actual value
    expire at -- the date at which a value expires (in seconds)
    ttl -- the touch TTL value (now + ttl > expire at, then update the
           expire at field)
    persist -- whether the data should be saved on disk or not
    created on -- the date when it was created
    last modified -- the date when it was first created or last updated
    last accessed -- the date when it was last accessed

Note: in memory the key and value are buffers with a size. On disk that size
is clearly specified in the structure.

**IMPORTANT NOTE:** The same value can be assigned under multiple keys, this
is a way to have multiple indexes. The concept is pretty simple: when I save
a sorted key for a row that needs to be updated in the HTML data, I need the
following info:

    <domain>:<priority>:<timestamp>:<id>

When the same row get updated before the previous update was processed, a
reference already exists in the journal, then the journal wants to update
that existing reference ("rename" the key). I can find that data using the
URI if we also have an index such as:

    <uri>

**WARNING:** the `<uri>` index needs to somehow ignore the "worked on" flag,
which means we could still have 2 entries at some point. And if the "worked on"
fails, the old row can still be deleted. So this is very specialized to our
journaling for update of rows in our Snap! database.

**NOTE:** For a single website, the `<uri>` key also has another issue: the
first N characters are going to all be the same (i.e. `http://example.com/`).

Note that to support an expiration timestamp, we also need another index
which is based on those timestamps.

    <timestamp>


# API

## Commands

The commands are sent using the library. We offer a CLI in order to let you
test the servers without your own code to make sure your installation works
as expected.

### Necessary Functions

The journal accepts the following commands:

* `NOP` (0) -- the system supports a "No Operation", mainly because we do not
  really want to use 0 as a command.

* `QUIT` (1) -- let the server know we want to terminate that connection
  cleanly.

* `SET` (2) -- set a new value, the command can be used to update an existing
  value; the command expect a key and a value.

* `GET` (3) -- return a value or undefined, the command expects the key used
  with the `SET` command. The `GET` can be used to scan the database, it can
  return multiple values, in this case, the `GET` also returns a cursor which
  can be specified in future `GET`. The `GET` also returns the time when the
  value expires, the data type, it may be asked not to return the data itself.
  (i.e. a probe). When multiple keys are returned, the return message includes
  the key.

* `DELETE` (4) -- delete a value if undefined.

### Nice to Have Functions

* `ADD` -- add a value to a numeric field, this is useful to avoid a `GET`+`SET`
  sequence which would require a manual `LOCK` and it can be implemented in
  a much faster way than with a `LOCK`.

* `NOT` -- flip a boolean value without the need for a `LOCK`.

* `TRUNCATE` -- resize a value to the specified size.

* `LOCK` -- lock a value; no one else can modify the value. A `GET` let the
  user know that the value is currently locked.

* `UNLOCK` -- unlock a value; let the backend we're done safely updating this
  value.

* `RENAME` -- rename a key; this is a fast GET/SET/DELETE. It can be useful
  for us when we update the same data and the key includes a date, then the
  old key needs to be updated with the new key (i.e. there is no need to
  update the same row twice).


## Message

The message is in binary since most values are likely going to be binary
data (in Snap! many Journals receive a URI but that's a particular case).

The data is just a stream so we do not swap any of the data. The header,
however, has its byte in Big Endian.

### Common Message (Client -> Server)

All messages have a common header:

    char[2] -- 'F' 'J'
    uint16_t -- version (1)
    uint16_t -- function (see commands above)
    uint16_t -- identifier (this packet ID so the client can match the reply)
    uint16_t -- flags/sizes
    uint64_t -- timestamp
    uint64_t -- timeout (optional, see flags)
    uint16_t -- count (optional, see flags)
    uint<size=8 or 16>_t -- key size
    char[<key size>] -- the key
    uint<size=8 or 16>_t -- key size (`GET` upper range)
    char[<key size>] -- the key (`GET` upper range)
    uint<size=0 or 8 or 16 or 32>_t -- value size
    uint8_t -- value type
    char[<value size> - 1] -- the value, may not be specified

Flags:

    bits    description
    0       size of key (0 - 8 bits, 1 - 16 bits)
    1-2     size of value (0 - no value, 1 - 8 bits, 2 - 16 bits, 3 - 32 bits)
    3       timeout specified
    4       update timeout
    5       overwrite (See below)
    6       worked on (See below)
    7-8     query (See `GET`)
    9       persist (i.e. save on disk)
    10      data (See `GET`)
    11      listen (See `SET`)
    12      persist (See `SET`)

**TODO:** define a set of flags specific to functions. One idea is to:

> Read the basic header (magic, version, function, flags) and then call
> the function and let the function deal with the rest of the message.
> That way all the flags, timestamp, etc. can be handled as expected by
> that one function.

Various flags are used differently depending on the command. See each command
for details.

The `timestamp` is used to know which command was emitted last. So if the
daemon receives a `SET` from three different clients, it will use the one
with the largest timestamp. This also allows us to know whether a `SET`
happens before or after a `DELETE`.

The `count` field is only there if the `key range` is set to 1. It defines
a maximum number of keys to be returned. If set to 0, the number is not
constrained.

### Common Message (Server -> Client)

The server reply varies widely depending on the command. For example, the
`GET` may return data when the `SET` just returns success or failure.

    char[2] -- 'J' 'R'
    uint16_t -- version (1)
    uint16_t -- identifier (same as identifier sent)
    uint16_t -- flags/sizes
    uint64_t -- timestamp
    uint64_t -- timeout (optional, see flags)
    uint16_t -- count (optional, see flags)
    uint<size=8 or 16>_t -- error size
    char[<key size>] -- the error

    `GET` may receive many key/value pairs; limit is 64K (see `count`)
    uint<size=8 or 16>_t -- key size
    char[<key size>] -- the key
    uint<size=0 or 8 or 16 or 32>_t -- value size
    uint8_t -- value type
    char[<value size> - 1] -- the value, may not be specified

**NOTE:** The timeout has a "touch" concept. Whenever we read a value, we want
to automatically extend its expiration date with a TTL. Only right now we do
not include a TTL in our message. We probably want to do that instead of just
a timestamp specifying a hard deadline because without such implementing a
"touch" concept becomes complicated.

### Data Types

The system supports the following types:

* void (0)
* buffer (1)
* string (2)
* integer (3)
* float (4)
* boolean (5)

The type is used mainly to make sure that the `SET` and `GET` make use of
the same data type. It's not otherwise enforced in any way. The `void`
type is used for an _empty data type_ (i.e. a type which doesn't have
any data).

The interest in having type is to allow commands such as `ADD` and `NOT`.
However, since there would be no way to enforce the type (i.e. we do not
have a schema) it is somewhat futile otherwise. Functions such as `ADD`
are useful when executed by the backend because that way we can make sure
that all `ADD` commands are applied and the total added reflects what the
client wanted (opposed to a `GET`, do your own math, `SET` where the client
need synchronization on its end).

### `NOP` Message

This message is to make 0 a valid function, but it does nothing. You should
never send/receive a `NOP` since it is totally useless to do so.

### `QUIT` Message

This is a message by the client to let the server know that the client is
about to close its connection.

### `SET` Message

The `SET` expects a key and a value. The timeout is optional.

If the `overwrite` flag is set to 0 and the key is already defined, then the
`SET` fails.

The flags can include a bit for "listening" to the value changes. In that case
the client is sent a message about this value whenver it is modified. This
gives us the ability to cache the value localy (on the client) up until the
time it gets modified. At that point the client can decide to either `GET`
the new value or just drop that cache.

The `persist` flag can be set to 0 or 1. If 1, the data gets saved on disk
and the reply to a `SET` is sent only after that worked. Some of our journals
must be persistent because a change has to be worked on, others do not need
persistence so much so they can be kept in RAM only and if you restart or
it crashes (unlikely ;-) ) then the key/value pair will be lost.

### `GET` Message

The `GET` expects a key and it returns the corresponding value or a "does
not exist" error.

The `GET` accepts a `query` flag:

* Key (0) -- the `GET` is used to request one specific key

* Key Range (1) -- this means two keys are defined. The function will return
  all the values in that range or a maximum number of key/value pairs as
  defined in the command.

* RegEx (2) -- this value means the key is regular expression.

* Cursor (3) -- the key represents a cursor, you can continue to read
  key/value pairs for a preceeding `GET` command

As specified in the flags, the `count` field is defined when a range is
defined. That represents the maximum number of key/value pairs returned in
a `GET`.

The `update timeout` flag, when set, means that the `timeout` gets bumped
to the `timeout` value. **TODO: that means we have two functions for the
timeout in the `GET` at this time, we need to revisit and see how to do
that cleanly.**

When you do a `GET`, you can define the `timeout` value. That value is
used to mark the value as being "worked on". This way you can skip values
currently being "worked on".

You can set the `worked on` flag to avoid doing a `GET` of a value already
marked as being worked on. The `worked on` status gets cleared after
a specified timeout.

The `data` flag tells us whether the data should be returned or not. If
set to 1, the data is included in the reply. **TBD:** we want to return the
type but not the data. The type is just one byte. We may also have 2 or 3
bits to know how to handle the return message.

### `DELETE` Message

The `DELETE` expects a key. It delets the value by saving its timestamp in
that value. The daemon will garbage collect later. This gives the client a
chance to reuse the value without having to reallocate it each time.

If the `overwrite` flag is set to 1, then the `DELETE` is immediate and
complete. The value is completely removed everywhere. This is useful if
you know that a key is either never going to be reused or very
unlikely (i.e. a key that includes a date, uses a UUID, etc.)


# TODO

Most everything...


# License

The source is covered by the MIT license. The debian folder is covered
by the GPL 2.0.


# Bugs

Submit bug reports and patches on
[github](https://github.com/m2osw/libfastjournal/issues).


_This file is part of the [snapcpp project](https://snapwebsites.org/)._
