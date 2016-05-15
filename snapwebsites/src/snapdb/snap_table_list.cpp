/*
 * Text:
 *      snap_table_list.cpp
 *
 * Description:
 *      Reads and describes a Snap database. This ease checking out the
 *      current content of the database as the cassandra-cli tends to
 *      show everything in hexadecimal number which is quite unpractical.
 *      Now we do it that way for runtime speed which is much more important
 *      than readability by humans, but we still want to see the data in an
 *      easy practical way which this tool offers.
 *
 * License:
 *      Copyright (c) 2012-2016 Made to Order Software Corp.
 *
 *      http://snapwebsites.org/
 *      contact@m2osw.com
 *
 *      Permission is hereby granted, free of charge, to any person obtaining a
 *      copy of this software and associated documentation files (the
 *      "Software"), to deal in the Software without restriction, including
 *      without limitation the rights to use, copy, modify, merge, publish,
 *      distribute, sublicense, and/or sell copies of the Software, and to
 *      permit persons to whom the Software is furnished to do so, subject to
 *      the following conditions:
 *
 *      The above copyright notice and this permission notice shall be included
 *      in all copies or substantial portions of the Software.
 *
 *      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 *      OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *      MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *      IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 *      CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 *      TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *      SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

// our lib
//
#include "snap_table_list.h"

// 3rd party libs
//
#include <QtCore>

snapTableList::snapTableList()
    //: f_tableName  -- auto-init
    //, f_canDrop    -- auto-init
    //, f_canDump    -- auto-init
    //, f_rowsToDump -- auto-init
{
}

void snapTableList::initList()
{
    if( f_list.isEmpty() )
    {
        // Set up defaults
        //
        addEntry("antihammering"           , true,  false );
        addEntry("backend"                 , true,  false );
        addEntry("branch"                  , true,  true  );
        addEntry("cache"                   , true,  false );
        addEntry("content"                 , true,  true  );
        addEntry("domains"                 , false, true  );
        addEntry("emails"                  , true,  true  );
        addEntry("epayment_paypal"         , true,  true  );
        addEntry("files"                   , true,  true  );
        addEntry("firewall"                , true,  false );
        addEntry("layout"                  , true,  true  );
        addEntry("lock_table"              , false, true  );
        addEntry("links"                   , true,  true  );
        addEntry("list"                    , true,  false );
        addEntry("listref"                 , true,  true  );
        addEntry("password"                , true,  true  );
        addEntry("processing"              , true,  true  );
        addEntry("revision"                , true,  true  );
        addEntry("secret"                  , true,  true  );
        addEntry("serverstats"             , true,  false );
        addEntry("sessions"                , true,  true  );
        addEntry("shorturl"                , true,  true  );
        addEntry("sites"                   , true,  true  );
        addEntry("test_results"            , true,  false );
        addEntry("tracker"                 , true,  false );
        addEntry("users"                   , true,  true  );
        addEntry("websites"                , false, true  );

        f_list["lock_table"].f_rowsToDump << "hosts";
    }
}

void snapTableList::overrideTablesToDump( const QStringList& tables_to_dump )
{
    for( auto & entry : f_list )
    {
        entry.f_canDump = false;
    }

    for( auto const & table_name : tables_to_dump )
    {
        f_list[table_name].f_canDump = true;
    }
}

QStringList snapTableList::tablesToDrop()
{
    QStringList the_list;
    for( auto const & entry : f_list )
    {
        if( !entry.f_canDrop ) continue;
        the_list << entry.f_tableName;
    }
    return the_list;
}

QStringList snapTableList::snapTableList::tablesToDump()
{
    QStringList the_list;
    for( auto const & entry : f_list )
    {
        if( !entry.f_canDump ) continue;
        the_list << entry.f_tableName;
    }
    return the_list;
}

bool snapTableList::canDumpRow( const QString& table_name, const QString& row_name )
{
    const auto& entry(f_list[table_name]);
    if( !entry.f_canDump )              return false;
    if( entry.f_rowsToDump.isEmpty() )  return true;
    return entry.f_rowsToDump.contains( row_name );
}

void snapTableList::addEntry( const QString& name, const bool can_drop, const bool can_dump )
{
    snapTableList entry;
    entry.f_tableName = name;
    entry.f_canDrop = can_drop;
    entry.f_canDump = can_dump;
    f_list[name] = entry;
}

snapTableList::name_to_list_t  snapTableList::f_list;

// vim: ts=4 sw=4 et
