// Copyright (c) 2021  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/project/snap-builder
// contact@m2osw.com
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
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

// self
//
#include    "snap_builder.h"
#include    "version.h"



// snaplogger lib
//
#include    <snaplogger/message.h>
#include    <snaplogger/options.h>


// advgetopt lib
//
#include    <advgetopt/exception.h>


// snapdev lib
//
#include    <snapdev/not_used.h>


// Qt lib
//
#include    <QtWidgets>
#include    <QFile>
#include    <QLabel>
#include    <QDir>


// boost lib
//
#include    <boost/preprocessor/stringize.hpp>


// C++ lib
//
#include    <fstream>


// C lib
//
#include    <stdlib.h>


// last include
//
#include    <snapdev/poison.h>




namespace
{

// no real options at the moment
const advgetopt::option g_options[] =
{
    advgetopt::define_option(
        advgetopt::Name("release-names")
      , advgetopt::Flags(advgetopt::any_flags<
            advgetopt::GETOPT_FLAG_GROUP_OPTIONS
          , advgetopt::GETOPT_FLAG_COMMAND_LINE
          , advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE
          , advgetopt::GETOPT_FLAG_CONFIGURATION_FILE>())
      , advgetopt::Help("Select a list of releases that are being built (xenial, bionic, etc) separated by commas.")
    ),
    advgetopt::end_options()
};

constexpr char const * const g_configuration_files[]
{
    "/etc/snapwebsites/snap-builder.conf",
    nullptr
};

// TODO: once we have stdc++20, remove all defaults
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
advgetopt::options_environment const g_options_environment =
{
    .f_project_name = "snap-builder",
    .f_group_name = "snapwebsites",
    .f_options = g_options,
    .f_options_files_directory = nullptr,
    .f_environment_variable_name = "SNAP_BUILDER",
    .f_configuration_files = g_configuration_files,
    .f_configuration_filename = nullptr,
    .f_configuration_directories = nullptr,
    .f_environment_flags = advgetopt::GETOPT_ENVIRONMENT_FLAG_PROCESS_SYSTEM_PARAMETERS,
    .f_help_header = "Usage: %p [-<opt>]\n"
                     "where -<opt> is one or more of:",
    .f_help_footer = "%c",
    .f_version = SNAPBUILDER_VERSION_STRING,
    .f_license = nullptr,
    .f_copyright = "Copyright (c) " BOOST_PP_STRINGIZE(UTC_BUILD_YEAR) "  Made to Order Software Corp.",
    .f_build_date = UTC_BUILD_DATE,
    .f_build_time = UTC_BUILD_TIME
};
#pragma GCC diagnostic pop



}
// noname namespace





namespace builder
{



snap_builder::snap_builder(int argc, char * argv[])
    : QMainWindow()
    , f_settings(this)
    , f_opt(g_options_environment)
    , f_communicator(ed::communicator::instance())
{
    snaplogger::add_logger_options(f_opt);
    f_opt.finish_parsing(argc, argv);
    snaplogger::process_logger_options(f_opt, "/etc/snapwebsites/logger");

    // TODO: use an option instead?
    //
    advgetopt::string_list_t segments;
    advgetopt::split_string(argv[0], segments, {"/"});
    bool found(false);
    for(auto s : segments)
    {
        if(s == "BUILD")
        {
            found = true;
            break;
        }
        if(!f_root_path.empty())
        {
            f_root_path += "/";
        }
        f_root_path += s;
    }
    if(!found)
    {
        std::cerr << "error: No \"BUILD\" found in your path, we do not know where the source root folder is located.\n";
        throw advgetopt::getopt_exit("No BUILD found in path. Can't locate source root folder.", 0);
    }
    if(f_root_path.empty())
    {
        f_root_path = ".";
    }

    f_qt_connection = std::make_shared<ed::qt_connection>();
    f_communicator->add_connection(f_qt_connection);

    setupUi(this);
    f_table->horizontalHeader()->setStretchLastSection(true);

    restoreGeometry(f_settings.value("geometry", saveGeometry()).toByteArray());
    restoreState(f_settings.value("state", saveState()).toByteArray());

    if(!f_opt.is_defined("verify"))
    {
        // ...
    }

    f_cache_path = getenv("HOME");
    f_cache_path += "/.cache/snapbuilder";

    {
        std::string cmd("mkdir -p ");
        cmd += f_cache_path;
        int const r(system(cmd.c_str()));
        if(r != 0)
        {
            SNAP_LOG_ERROR
                << "could not create folder \""
                << f_cache_path
                << "\"."
                << SNAP_LOG_SEND;
            throw std::runtime_error("could not create cache folder");
        }
    }

    // TODO: do that after n secs. so the UI is up
    //
    read_list_of_projects();
}


snap_builder::~snap_builder()
{
}


void snap_builder::run()
{
    f_communicator->run();
}


std::string const & snap_builder::get_root_path() const
{
    return f_root_path;
}


std::string const & snap_builder::get_cache_path() const
{
    return f_cache_path;
}


std::string const & snap_builder::get_launchpad_url() const
{
    return f_launchpad_url;
}


advgetopt::string_list_t const & snap_builder::get_release_names() const
{
    return f_release_names;
}


void snap_builder::closeEvent(QCloseEvent * event)
{
    QMainWindow::closeEvent(event);

    f_communicator->remove_connection(f_qt_connection);
    f_qt_connection.reset();

    f_settings.setValue("geometry", saveGeometry());
    f_settings.setValue("state", saveState());
}


void snap_builder::read_list_of_projects()
{
    std::string path(f_root_path);
    path += "/BUILD/Debug/deps.make";

    std::ifstream deps;
    deps.open(path);
    if(!deps.is_open())
    {
        // TODO: A message box will currently fail on load...
        QMessageBox(
              QMessageBox::Critical
            , "Dependencies Not Found"
            , QString("The list of dependencies could not be read from ")
                + QString::fromUtf8(path.c_str())
                + "\""
            , QMessageBox::Close
            , this
            , Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
        return;
    }

    int line(1);
    std::string s;
    while(std::getline(deps, s))
    {
        // ignore empty lines and comments
        //
        if(s.empty()
        || s[0] == '#')
        {
            continue;
        }

        std::string::size_type const colon(s.find(':'));
        if(colon == std::string::npos)
        {
            std::cerr
                << "error:"
                << path
                << ":"
                << line
                << ": no ':' found on the line.\n";
            continue;
        }

        std::string const name(s.substr(0, colon));
        advgetopt::string_list_t dep_list;
        advgetopt::split_string(s.substr(colon + 1), dep_list, {" "});
        project::pointer_t p(std::make_shared<project>(this, name, dep_list));
        f_projects.push_back(p);
    }

    project::sort(f_projects);

    int const count(std::count_if(
                  f_projects.begin()
                , f_projects.end()
                , [](project::pointer_t p) { return p->is_valid(); }));
    f_table->setRowCount(count);

    int row(0);
    for(auto const & p : f_projects)
    {
        if(p->is_valid())
        {
            f_table->setItem(row, 0, new QTableWidgetItem(QString::fromUtf8(p->get_name().c_str())));
            f_table->setItem(row, 1, new QTableWidgetItem(QString::fromUtf8(p->get_version().c_str())));
            f_table->setItem(row, 2, new QTableWidgetItem("-")); // TODO
            f_table->setItem(row, 3, new QTableWidgetItem(QString::fromUtf8(p->get_state().c_str())));
            f_table->setItem(row, 4, new QTableWidgetItem(QString::fromUtf8(p->get_last_commit_as_string().c_str())));
            f_table->setItem(row, 5, new QTableWidgetItem("-")); // TODO

            ++row;
        }
    }

    adjust_columns();
}


void snap_builder::adjust_columns()
{
    int const max(f_table->columnCount());
    for(int col(0); col < max; ++col)
    {
        f_table->resizeColumnToContents(col);
    }
}


void snap_builder::on_refresh_list_triggered()
{
    read_list_of_projects();
}


void snap_builder::on_build_release_triggered()
{
    // TODO: make it output in a Qt window and prevent doubling the call...
    //
    std::string cmd("make -C ");
    cmd += f_root_path;
    cmd += "/BUILD/Release &";
    std::cout << "command: " << cmd << "\n";
    system(cmd.c_str());
}


void snap_builder::on_build_debug_triggered()
{
    // TODO: make it output in a Qt window and prevent doubling the call...
    //
    std::string cmd("make -C ");
    cmd += f_root_path;
    cmd += "/BUILD/Debug &";
    std::cout << "command: " << cmd << "\n";
    system(cmd.c_str());
}


void snap_builder::on_action_quit_triggered()
{
    close();
}


} // builder namespace
// vim: ts=4 sw=4 et
