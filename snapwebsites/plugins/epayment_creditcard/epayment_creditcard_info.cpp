// Snap Websites Server -- epayment_creditcard_info_t implementation
// Copyright (C) 2011-2016  Made to Order Software Corp.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA


/** \file
 * \brief The implementation of epayment_creditcard_info_t.
 *
 * This file contains the implementation of the epayment_creditcard_info_t
 * class.
 *
 * The class is used whenever a client sends his credit card information.
 * It is expected that the current credit card processing facility that
 * you offer be sent that information to actually charge the client's
 * credit card.
 */

#include "epayment_creditcard.h"

//#include "compression.h"
//#include "dbutils.h"
//#include "log.h"
//
//#include <csspp/assembler.h>
//#include <csspp/compiler.h>
//#include <csspp/exceptions.h>
//#include <csspp/parser.h>
//
//#include <iostream>

#include "poison.h"


SNAP_PLUGIN_EXTENSION_START(epayment_creditcard)


void epayment_creditcard_info_t::set_creditcard_number(QString const & creditcard_number)
{
    f_creditcard_number = creditcard_number;
}


QString epayment_creditcard_info_t::get_creditcard_number() const
{
    return f_creditcard_number;
}


void epayment_creditcard_info_t::set_security_code(QString const & security_code)
{
    f_security_code = security_code;
}


QString epayment_creditcard_info_t::get_security_code() const
{
    return f_security_code;
}


void epayment_creditcard_info_t::set_expiration_date_month(QString const & expiration_date_month)
{
    f_expiration_date_month = expiration_date_month;
}


QString epayment_creditcard_info_t::get_expiration_date_month() const
{
    return f_expiration_date_month;
}


void epayment_creditcard_info_t::set_expiration_date_year(QString const & expiration_date_year)
{
    f_expiration_date_year = expiration_date_year;
}


QString epayment_creditcard_info_t::get_expiration_date_year() const
{
    return f_expiration_date_year;
}


void epayment_creditcard_info_t::set_user_name(QString const & user_name)
{
    f_user_name = user_name;
}


QString epayment_creditcard_info_t::get_user_name() const
{
    return f_user_name;
}


void epayment_creditcard_info_t::set_address1(QString const & address1)
{
    f_address1 = address1;
}


QString epayment_creditcard_info_t::get_address1() const
{
    return f_address1;
}


void epayment_creditcard_info_t::set_address2(QString const & address2)
{
    f_address2 = address2;
}


QString epayment_creditcard_info_t::get_address2() const
{
    return f_address2;
}


void epayment_creditcard_info_t::set_city(QString const & city)
{
    f_city = city;
}


QString epayment_creditcard_info_t::get_city() const
{
    return f_city;
}


void epayment_creditcard_info_t::set_province(QString const & province)
{
    f_province = province;
}


QString epayment_creditcard_info_t::get_province() const
{
	return f_province;
}


void epayment_creditcard_info_t::set_postal_code(QString const & postal_code)
{
	f_postal_code = postal_code;
}


QString epayment_creditcard_info_t::get_postal_code() const
{
	return f_postal_code;
}


void epayment_creditcard_info_t::set_country(QString const & country)
{
	f_country = country;
}


QString epayment_creditcard_info_t::get_country() const
{
	return f_country;
}


SNAP_PLUGIN_EXTENSION_END()

// vim: ts=4 sw=4 et
