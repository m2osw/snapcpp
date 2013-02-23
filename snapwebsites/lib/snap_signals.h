// Snap Websites Servers -- signals
// Copyright (C) 2011  Made to Order Software Corp.
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
#ifndef SNAP_SIGNAL_H
#define SNAP_SIGNAL_H

#include <boost/signals2.hpp>

/** \brief Define a named signal with given parameters.
 *
 * This macro is used to quickly define a signal that other users can
 * listen to.
 *
 * For example, the bootstrap and title signals are defined as:
 *
 * \code
 * SNAP_SIGNAL(bootstrap, (), ())
 * SNAP_SIGNAL(title, (const std::string& name), (name))
 * \endcode
 *
 * The first macro parameter is the signal name. The macro creates a signal
 * typedef named form signal_\<name>, a function named signal_listen_\<name>
 *
 *
 * \param[in] name  The name of the signal.
 * \param[in] parameters  A list of parameters written between parenthesis.
 * \param[in] variables  List the variable names as they appear in the parameters.
 */
#define	SNAP_SIGNAL(name, parameters, variables) \
	typedef boost::signals2::signal<void parameters> signal_##name; \
	boost::signals2::connection signal_listen_##name(const signal_##name::slot_type& slot) \
	   { return f_signal_##name.connect(slot); } \
	private: signal_##name f_signal_##name; bool name##_impl parameters; \
	public: void name parameters { if(name##_impl variables) f_signal_##name variables; }



#endif
// SNAP_SIGNAL_H
// vim: ts=4 sw=4
