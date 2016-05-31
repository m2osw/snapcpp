//===============================================================================
// Copyright (c) 2005-2016 by Made to Order Software Corporation
// 
// All Rights Reserved.
// 
// The source code in this file ("Source Code") is provided by Made to Order Software Corporation
// to you under the terms of the GNU General Public License, version 2.0
// ("GPL").  Terms of the GPL can be found in doc/GPL-license.txt in this distribution.
// 
// By copying, modifying or distributing this software, you acknowledge
// that you have read and understood your obligations described above,
// and agree to abide by those obligations.
// 
// ALL SOURCE CODE IN THIS DISTRIBUTION IS PROVIDED "AS IS." THE AUTHOR MAKES NO
// WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
// COMPLETENESS OR PERFORMANCE.
//===============================================================================

#include "WebsiteModel.h"
#include <snapwebsites/snapwebsites.h>

#include <libtld/tld.h>

#include <QMessageBox>
#include <QSettings>
#include <QVariant>

#include <iostream>
#include <exception>

#include "poison.h"

using namespace QtCassandra;


WebsiteModel::WebsiteModel()
{
}


void WebsiteModel::doQuery()
{
    QString const context_name(snap::get_name(snap::name_t::SNAP_NAME_CONTEXT));
    QString const table_name(snap::get_name(snap::name_t::SNAP_NAME_WEBSITES));

    auto q = QCassandraQuery::create(f_session);
    q->query(
        QString("SELECT DISTINCT key FROM %1.%2")
            .arg(context_name)
            .arg(table_name)
        );
    q->setPagingSize( 100 );

    QueryModel::doQuery( q );
}


bool WebsiteModel::fetchFilter( const QByteArray& key )
{
    if( !QueryModel::fetchFilter( key ) )
    {
        return false;
    }

    QString const row_index_name(snap::get_name(snap::name_t::SNAP_NAME_INDEX));
    if( key == row_index_name )
    {
        // Ignore *index* entries
        return false;
    }

    const char *d = key.data();
    tld_info info;
    tld_result r( tld( d, &info ) );
    //
    if( r == TLD_RESULT_SUCCESS )
    {
        const char *domain = d; // by default assume no sub-domain
        for(; d < info.f_tld; ++d)
        {
            if(*d == '.')
            {
                domain = d + 1;
            }
        }

        return domain == f_domain_org_name;
    }

    return false;
}


#if 0
QVariant WebsiteModel::data( QModelIndex const & idx, int role ) const
{
    if( role == Qt::UserRole )
    {
        return QueryModel::data( idx, role );
    }

    if( role != Qt::DisplayRole && role != Qt::EditRole )
    {
        return QVariant();
    }

    if( static_cast<int>(f_rows.size()) <= idx.row() )
    {
        return QVariant();
    }

    const QByteArray& row( f_rows[idx.row()] );
    tld_result r;
    tld_info info;
    const char *d = row.data();
    r = tld( d, &info );
    if( r != TLD_RESULT_SUCCESS )
    {
        QMessageBox::critical
                ( 0
                , "Invalid TLD in Domain Name"
                , QString("The TLD of this domain: \"%1\" is not valid. This entry will be skipped.")
                          .arg(row.data())
                , QMessageBox::Ok
                );
        return QVariant();
    }
    const char *domain = d; // by default assume no sub-domain
    for(; d < info.f_tld; ++d)
    {
        if(*d == '.')
        {
            domain = d + 1;
        }
    }

    //const int mid_pos(f_domain_org_name.length() + 2);
    //if( key.length() <= mid_pos)
    if( domain != f_domain_org_name )
    {
        // note that the length of the key is at least 4 additional
        // characters but at this point we don't make sure that the
        // key itself is fully correct (it should be)
        QMessageBox::warning
                ( 0
                  , tr("Invalid Website Index")
                  , tr("Somehow we have found an invalid entry in the list of websites. It is suggested that you regenerate the index. Note that this index is not used by the Snap server itself.")
                  , QMessageBox::Ok
                  );
        return QueryModel::data( idx, role );
    }

    return row;
}
#endif


// vim: ts=4 sw=4 et
