// Snap Websites Server -- advanced handling of Unix processes
// Copyright (C) 2013  Made to Order Software Corp.
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
#ifndef SNAP_PROCESS_H
#define SNAP_PROCESS_H

#include <QString>
#include <QVector>
#include <controlled_vars/controlled_vars_auto_init.h>
#include <controlled_vars/controlled_vars_auto_ptr_init.h>

namespace snap
{


class process
{
public:
    enum mode_t
    {
        PROCESS_MODE_COMMAND,
        PROCESS_MODE_INPUT,
        PROCESS_MODE_OUTPUT,
        PROCESS_MODE_INOUT,
        PROCESS_MODE_INOUT_INTERACTIVE
    };
    typedef controlled_vars::limited_auto_init<mode_t, PROCESS_MODE_COMMAND, PROCESS_MODE_INOUT_INTERACTIVE, PROCESS_MODE_COMMAND> zmode_t;

    class process_output_callback
    {
    public:
        virtual bool                output_available(process *p, const QString& output) = 0;
    };
    typedef controlled_vars::ptr_auto_init<process_output_callback> zpprocess_output_callback_t;

                                process(const QString& name);

    const QString&              get_name() const;

    // setup the process
    void                        set_mode(mode_t mode);
    void                        set_forced_environment(bool forced = true);

    void                        set_command(const QString& name);
    void                        add_argument(const QString& arg);
    void                        add_environ(const QString& env);

    int                         run();

    // what is sent to the command stdin
    void                        set_input(const QString& input);

    // what is received from the command stdout
    QString                     get_output(bool reset = false) const;
    void                        set_output_callback(process_output_callback *callback);

private:
    // prevent copies
    process(const process& rhs);
    process& operator = (const process& rhs);

    const QString               f_name;
    zmode_t                     f_mode;
    QString                     f_command;
    QStringList                 f_arguments;
    QStringList                 f_environment;
    QString                     f_input;
    QString                     f_output;
    controlled_vars::fbool_t    f_forced_environment;
    zpprocess_output_callback_t f_output_callback;
};

} // namespace snap
#endif
// SNAP_PROCESS_H
// vim: ts=4 sw=4 et
