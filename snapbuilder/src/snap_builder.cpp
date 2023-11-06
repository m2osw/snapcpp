// Copyright (c) 2021-2023  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/project/snapbuilder
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

#include    "about_dialog.h"
#include    "project.h"
#include    "version.h"



// snaplogger lib
//
#include    <snaplogger/message.h>
#include    <snaplogger/options.h>


// cppprocess lib
//
#include    <cppprocess/io_capture_pipe.h>


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
#include    <sys/stat.h>
#include    <unistd.h>


// last include
//
#include    <snapdev/poison.h>




namespace
{

// no real options at the moment
const advgetopt::option g_options[] =
{
    advgetopt::define_option(
        advgetopt::Name("distribution")
      , advgetopt::Flags(advgetopt::any_flags<
            advgetopt::GETOPT_FLAG_GROUP_OPTIONS
          , advgetopt::GETOPT_FLAG_COMMAND_LINE
          , advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE
          , advgetopt::GETOPT_FLAG_CONFIGURATION_FILE>())
      , advgetopt::Help("Define the name of the distribution to use when clicking the Bump Version button (and automatic rebuild of the tree).")
    ),
    advgetopt::define_option(
        advgetopt::Name("release-names")
      , advgetopt::Flags(advgetopt::any_flags<
            advgetopt::GETOPT_FLAG_GROUP_OPTIONS
          , advgetopt::GETOPT_FLAG_COMMAND_LINE
          , advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE
          , advgetopt::GETOPT_FLAG_CONFIGURATION_FILE>())
      , advgetopt::Help("Select a list of releases that are being built (xenial, bionic, etc) separated by commas.")
    ),
    advgetopt::define_option(
        advgetopt::Name("verify")
      , advgetopt::Flags(advgetopt::any_flags<
            advgetopt::GETOPT_FLAG_GROUP_OPTIONS
          , advgetopt::GETOPT_FLAG_COMMAND_LINE
          , advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE
          , advgetopt::GETOPT_FLAG_CONFIGURATION_FILE>())
      , advgetopt::Help("[NOT IMPLEMENTED] Verify as much as possible that everything is as expected before running a build.")
    ),
    advgetopt::end_options()
};

constexpr char const * const g_configuration_files[]
{
    "/etc/snapwebsites/snapbuilder.conf",
    nullptr
};

// TODO: once we have stdc++20, remove all defaults
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
advgetopt::options_environment const g_options_environment =
{
    .f_project_name = "snapbuilder",
    .f_group_name = "snapwebsites",
    .f_options = g_options,
    .f_options_files_directory = nullptr,
    .f_environment_variable_name = "SNAP_BUILDER",
    .f_environment_variable_intro = nullptr,
    .f_section_variables_name = nullptr,
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
    if(!snaplogger::process_logger_options(
                  f_opt
                , "/etc/snapwebsites/logger"
                , std::cout
                , false))       // avoid the banner by default
    {
        // exit on any error
        throw advgetopt::getopt_exit("logger options generated an error.", 0);
    }

    // TODO: use an option instead?
    // (also somehow this fails in gdb!?)
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
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wrestrict"
        f_root_path = ".";
#pragma GCC diagnostic pop
    }

    f_qt_connection = std::make_shared<ed::qt_connection>();
    f_communicator->add_connection(f_qt_connection);

    setupUi(this);
    f_table->horizontalHeader()->setStretchLastSection(true);
    f_table->setSelectionBehavior(QTableWidget::SelectRows);
    f_table->setSelectionMode(QTableWidget::SingleSelection);

    setWindowIcon(QIcon(":/icons/icon.png"));

    restoreGeometry(f_settings.value("geometry", saveGeometry()).toByteArray());
    restoreState(f_settings.value("state", saveState()).toByteArray());

    if(f_opt.is_defined("distribution"))
    {
        f_distribution = f_opt.get_string("distribution");
    }

    if(!f_opt.is_defined("verify"))
    {
        // ... what did I really want to verify with a global flag?
    }

    f_config_path = getenv("HOME");
    f_config_path += "/.config/snapbuilder";

    {
        std::string cmd("mkdir -p ");
        cmd += f_config_path;
        int const r(system(cmd.c_str()));
        if(r != 0)
        {
            SNAP_LOG_FATAL
                << "could not create folder \""
                << f_config_path
                << "\"."
                << SNAP_LOG_SEND;
            throw std::runtime_error("could not create config folder");
        }
    }

    f_cache_path = getenv("HOME");
    f_cache_path += "/.cache/snapbuilder";

    {
        std::string cmd("mkdir -p ");
        cmd += f_cache_path;
        int const r(system(cmd.c_str()));
        if(r != 0)
        {
            SNAP_LOG_FATAL
                << "could not create folder \""
                << f_cache_path
                << "\"."
                << SNAP_LOG_SEND;
            throw std::runtime_error("could not create cache folder");
        }
    }

    if(f_opt.is_defined("launchpad-url"))
    {
        f_launchpad_url = f_opt.get_string("launchpad-url");
    }

    // TODO: do that after n secs. so the UI is up
    //
    read_list_of_projects();

    on_generate_dependency_svg_triggered();

    f_timer_id = startTimer(1000 * 60); // 1 minute interval
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


void snap_builder::timerEvent(QTimerEvent * timer_event)
{
    snapdev::NOT_USED(timer_event);

    QTableWidgetItem * item(nullptr);
    int row(0);
    for(auto const & p : f_projects)
    {
        if(p->is_valid())
        {
            if(p->get_building())
            {
                if(p->retrieve_ppa_status())
                {
                    p->load_remote_data();

                    item = f_table->item(row, 2);
                    item->setText(QString::fromUtf8(p->get_remote_version().c_str()));

                    item = f_table->item(row, 3);
                    item->setText(QString::fromUtf8(p->get_state().c_str()));

                    item = f_table->item(row, 5);
                    item->setText(QString::fromUtf8(p->get_remote_build_state().c_str()));

                    item = f_table->item(row, 6);
                    item->setText(QString::fromUtf8(p->get_remote_build_date().c_str()));

                    update_state(row);
                }
            }
            ++row;
        }
    }
}


/** \brief This function computes a state for each row (project).
 *
 * The state of a project is determined by the project. It results
 * in a color and a string and represents what we can do next with
 * that object.
 */
void snap_builder::update_state(int row)
{
    QTableWidgetItem * item(f_table->item(row, 0));
    QVariant const v(item->data(Qt::UserRole));
    project::pointer_t p(v.value<project_ptr>().f_ptr);
    if(p == nullptr)
    {
        // this should never happen
        //
        return;
    }

    std::string state(p->get_state());
    if(state.empty())
    {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wrestrict"
        state = "?";
#pragma GCC diagnostic pop
    }

    // a default brush represents the default background color
    //
    std::string unknown;
    QBrush background;
    switch(state[0])
    {
    case 'b':
        if(state == "bad version")
        {
            // there are changes in your local version but the version is
            // the same as a successful build on the remote (i.e. you need
            // to click on "Edit Changelog")
            //
            background = QColor(222, 119, 153);
        }
        else if(state == "building")
        {
            // the project is being built right now
            //
            background = QColor(211, 255, 78);
        }
        else if(state == "build failed")
        {
            // the last build failed
            //
            background = QColor(255, 225, 225);
        }
        else if(state == "built")
        {
            // the last build succeeded and we do not have changes on our end
            //
            background = QColor(240, 255, 240);
        }
        else
        {
            unknown = state;
        }
        break;

    case 'n':
        if(state == "never built")
        {
            // this means we never got info from the remote (or the file
            // is empty) and that means it was never built there
            //
            background = QColor(200, 200, 200);
        }
        else if(state == "not committed")
        {
            background = QBrush(QColor(255, 248, 240));
        }
        else if(state == "not pushed")
        {
            background = QBrush(QColor(255, 240, 230));
        }
        else
        {
            unknown = state;
        }
        break;

    case 'r':
        if(state == "ready")
        {
            // this is the default
            //
            background = QBrush();
        }
        else
        {
            unknown = state;
        }
        break;

    }

    if(!unknown.empty())
    {
        SNAP_LOG_WARNING
            << "unknown (unhandled) project state: \""
            << unknown
            << "\"."
            << SNAP_LOG_SEND;
    }

    // update the background of the entire row
    //
    int const max(f_table->columnCount());
    for(int col(0); col < max; ++col)
    {
        QTableWidgetItem * cell(f_table->item(row, col));
        cell->setBackground(background);
    }
}


// TODO: implement a version where we only update one project, which would make
//       it a lot faster
//
void snap_builder::read_list_of_projects()
{
    statusbar->showMessage("Reading list of projects...");

    std::string path(get_root_path());
    path += "/BUILD/Debug/deps.make";

    std::ifstream deps;
    deps.open(path);
    if(!deps.is_open())
    {
        // TODO: A message box will currently fail on load...
        QMessageBox msg(
              QMessageBox::Critical
            , "Dependencies Not Found"
            , QString("The list of dependencies could not be read from ")
                + QString::fromUtf8(path.c_str())
                + "\""
            , QMessageBox::Close
            , this
            , Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
        msg.exec();
        return;
    }

    std::string reselect;
    if(f_current_project != nullptr)
    {
        reselect = f_current_project->get_name();
        f_current_project.reset();
    }

    f_projects.clear();

    int line(1);
    std::string first;
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

        if(first.empty())
        {
            first = s;
        }
        else if(first == s)
        {
            // TODO: fix the cmake that generates this file, once in a while
            //       it duplicates the output without first clearing the
            //       file (i.e. because we use an append)
            //
            SNAP_LOG_ERROR
                << path
                << ":"
                << line
                << ": repeat of first line found in the dependencies file.\n"
                << SNAP_LOG_SEND;
            break;
        }

        std::string::size_type const colon(s.find(':'));
        if(colon == std::string::npos)
        {
            SNAP_LOG_ERROR
                << path
                << ":"
                << line
                << ": no ':' found on the line.\n"
                << SNAP_LOG_SEND;
            continue;
        }

        std::string const name(s.substr(0, colon));
        advgetopt::string_list_t dep_list;
        advgetopt::split_string(s.substr(colon + 1), dep_list, {" "});
        project::pointer_t p(std::make_shared<project>(this, name, dep_list));
        f_projects.push_back(p);
    }
    project::simplify(f_projects);

    project::sort(f_projects);

    f_table->clearContents();   // restart from scratch

    int const count(std::count_if(
                  f_projects.begin()
                , f_projects.end()
                , [](project::pointer_t p) { return p->is_valid(); }));
    f_table->setRowCount(count);

    QTableWidgetItem * item(nullptr);
    int row(0);
    int reselect_row(-1);
    for(auto const & p : f_projects)
    {
        if(p->is_valid())
        {
            project_ptr ptr({p});
            QVariant v(QVariant::fromValue(ptr));

            if(p->get_name() == reselect)
            {
                reselect_row = row;
                f_current_project = p;
            }

            item = new QTableWidgetItem(QString::fromUtf8(p->get_name().c_str()));
            item->setData(Qt::UserRole, v);
            f_table->setItem(row, 0, item);

            item = new QTableWidgetItem(QString::fromUtf8(p->get_version().c_str()));
            item->setData(Qt::UserRole, v);
            f_table->setItem(row, 1, item);

            item = new QTableWidgetItem(QString::fromUtf8(p->get_remote_version().c_str()));
            item->setData(Qt::UserRole, v);
            f_table->setItem(row, 2, item);

            item = new QTableWidgetItem(QString::fromUtf8(p->get_state().c_str()));
            item->setData(Qt::UserRole, v);
            f_table->setItem(row, 3, item);

            item = new QTableWidgetItem(QString::fromUtf8(p->get_last_commit_as_string().c_str()));
            item->setData(Qt::UserRole, v);
            f_table->setItem(row, 4, item);

            item = new QTableWidgetItem(QString::fromUtf8(p->get_remote_build_state().c_str()));
            item->setData(Qt::UserRole, v);
            f_table->setItem(row, 5, item);

            item = new QTableWidgetItem(QString::fromUtf8(p->get_remote_build_date().c_str()));
            item->setData(Qt::UserRole, v);
            f_table->setItem(row, 6, item);

            update_state(row);

            ++row;
        }
    }

    adjust_columns();

    if(reselect_row != -1)
    {
        f_table->selectRow(reselect_row);
    }

    set_button_status();

    statusbar->clearMessage();
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


void snap_builder::on_refresh_clicked()
{
    if(f_current_project == nullptr)
    {
        QMessageBox msg(
              QMessageBox::Critical
            , "No Selection"
            , QString("The Refresh button requires a project to be selected to work.")
            , QMessageBox::Close
            , const_cast<snap_builder *>(this)
            , Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
        msg.exec();
        return;
    }

    f_current_project->retrieve_ppa_status();
}


void snap_builder::on_coverage_clicked()
{
    std::string const selection(get_selection_with_path());
    if(selection.empty())
    {
        return;
    }

    statusbar->showMessage("Running coverage...");

    std::string cmd("cd ");
    cmd += selection;
    cmd += " && ./mk -c";
    int const r(system(cmd.c_str()));
    if(r != 0)
    {
        QMessageBox msg(
              QMessageBox::Critical
            , "Coverage Run Failed"
            , "The ./mk command \""
                + QString::fromUtf8(cmd.c_str())
                + "\" failed. See your console for details."
            , QMessageBox::Close
            , this
            , Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
        msg.exec();
    }

    statusbar->clearMessage();
}


void snap_builder::on_build_release_triggered()
{
    statusbar->showMessage("Build Release version of the entire Snap! C++ environment...");

    // TODO: make it output in a Qt window and prevent doubling the call...
    //
    std::string cmd("make -C ");
    cmd += get_root_path();
    cmd += "/BUILD/Release &";
    std::cout << "\n-----------------------------------------\ncommand: " << cmd << "\n";
    int const r(system(cmd.c_str()));
    if(r != 0)
    {
        SNAP_LOG_ERROR
            << "make command failed: \""
            << cmd
            << "\"."
            << SNAP_LOG_SEND;
    }

    statusbar->clearMessage();
}


void snap_builder::on_build_debug_triggered()
{
    statusbar->showMessage("Build Debug version of the entire Snap! C++ environment...");

    // TODO: make it output in a Qt window and prevent doubling the call...
    //
    std::string cmd("make -C ");
    cmd += get_root_path();
    cmd += "/BUILD/Debug &";
    std::cout << "\n-----------------------------------------\ncommand: " << cmd << "\n";
    int const r(system(cmd.c_str()));
    if(r != 0)
    {
        SNAP_LOG_ERROR
            << "make command failed: \""
            << cmd
            << "\"."
            << SNAP_LOG_SEND;
    }

    statusbar->clearMessage();
}


void snap_builder::on_build_sanitize_triggered()
{
    statusbar->showMessage("Build Sanitize version of the entire Snap! C++ environment...");

    // TODO: make it output in a Qt window and prevent doubling the call...
    //
    std::string cmd("make -C ");
    cmd += get_root_path();
    cmd += "/BUILD/Sanatize &";
    std::cout << "\n-----------------------------------------\ncommand: " << cmd << "\n";
    int const r(system(cmd.c_str()));
    if(r != 0)
    {
        SNAP_LOG_ERROR
            << "make command failed: \""
            << cmd
            << "\"."
            << SNAP_LOG_SEND;
    }

    statusbar->clearMessage();
}


void snap_builder::on_mark_build_done_triggered()
{
    if(f_current_project == nullptr)
    {
        QMessageBox msg(
              QMessageBox::Critical
            , "No Project Selected"
            , "To clear a project's build status, a project needs to be selected."
            , QMessageBox::Close
            , const_cast<snap_builder *>(this)
            , Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
        msg.exec();
        return;
    }

std::cerr << "on_mark_build_done_triggered() called!\n";
    f_current_project->mark_as_done_building();
}


void snap_builder::on_generate_dependency_svg_triggered()
{
    statusbar->showMessage("Generating SVG of dependencies...");

    project::generate_svg(
              f_projects
            , std::bind(&snap_builder::svg_ready, this, std::placeholders::_1, std::placeholders::_2));
}


bool snap_builder::svg_ready(cppprocess::io * output_pipe, cppprocess::done_reason_t reason)
{
    if(reason != cppprocess::done_reason_t::DONE_REASON_EOF
    && reason != cppprocess::done_reason_t::DONE_REASON_HUP)
    {
        SNAP_LOG_ERROR
            << "error: dot command failed; reason: "
            << static_cast<int>(reason)
            << "\n"
            << SNAP_LOG_SEND;
        return false;
    }

    cppprocess::io_capture_pipe * capture(dynamic_cast<cppprocess::io_capture_pipe *>(output_pipe));
    if(capture == nullptr)
    {
        std::cerr << "error: could not get the output capture pipe.\n";
        return false;
    }

    // TODO: should this be Debug or another sub-directory or yet another directory?
    //
    std::string const svg_filename(get_root_path() + "/BUILD/Debug/clean-dependencies.svg");

    {
        std::ofstream out;
        out.open(svg_filename);
        if(!out.is_open())
        {
            // Make this a GUI error
            std::cerr << "error: could not open " << svg_filename << "\n";
            return false;
        }
        std::string svg(capture->get_output());
        out.write(svg.c_str(), svg.size());
    }

    dependency_tree->load(QString::fromUtf8(svg_filename.c_str()));

    statusbar->clearMessage();

    return true;
}


void snap_builder::on_action_quit_triggered()
{
    close();
}


void snap_builder::on_about_snapbuilder_triggered()
{
    AboutDialog about(this);
    about.exec();
}


void snap_builder::on_f_table_clicked(QModelIndex const & index)
{
    QList<QTableWidgetItem *> items(f_table->selectedItems());
    if(items.empty())
    {
        // this should not happen, but just in case
        //
        f_current_project.reset();
    }
    else
    {
        QVariant const v(index.data(Qt::UserRole));
        f_current_project = v.value<project_ptr>().f_ptr;
    }

    set_button_status();
}


void snap_builder::set_button_status()
{
    if(f_current_project == nullptr)
    {
        f_current_selection->setText("No Selection");
        build_package->setEnabled(false);
        meld->setEnabled(false);
        edit_changelog->setEnabled(false);
        edit_control->setEnabled(false);
        bump_version->setEnabled(false);
        local_compile->setEnabled(false);
        run_tests->setEnabled(false);
        git_commit->setEnabled(false);
        git_push->setEnabled(false);
        git_pull->setEnabled(false);
        refresh->setEnabled(false);
        coverage->setEnabled(false);
    }
    else
    {
        // TODO: test everything necessary to properly set the status of the
        //       buttons and not just all enabled...
        //       (i.e. the version/state on Launchpad are important)

        f_current_selection->setText(QString::fromUtf8(f_current_project->get_name().c_str()));

        std::string const & state(f_current_project->get_state());
        build_package->setEnabled(state == "ready" || state == "never built");
        meld->setEnabled(true);
        edit_changelog->setEnabled(true);
        bump_version->setEnabled(true);
        edit_control->setEnabled(true);
        local_compile->setEnabled(true);
        run_tests->setEnabled(true);
        git_commit->setEnabled(state == "not committed");
        git_push->setEnabled(state == "not pushed");
        git_pull->setEnabled(state == "ready");
        refresh->setEnabled(true);
        coverage->setEnabled(true);
    }
}


std::string snap_builder::get_selection() const
{
    if(f_current_project == nullptr)
    {
        return std::string();
    }

    return f_current_project->get_name();
}


std::string snap_builder::get_selection_with_path() const
{
    std::string path(get_selection());
    if(path.empty())
    {
        return path;
    }

    std::string root_path(get_root_path());

    std::string const top_dir(root_path + '/' + path);
    struct stat s;
    if(stat(top_dir.c_str(), &s) == 0
    && S_ISDIR(s.st_mode))
    {
        return top_dir;
    }

    std::string const contrib_dir(root_path + "/contrib/" + path);
    if(stat(contrib_dir.c_str(), &s) == 0
    && S_ISDIR(s.st_mode))
    {
        return contrib_dir;
    }

    QMessageBox msg(
          QMessageBox::Critical
        , "Project Directory Not Found"
        , QString("We could not find the directory for project \"")
            + QString::fromUtf8(path.c_str())
            + "\""
        , QMessageBox::Close
        , const_cast<snap_builder *>(this)
        , Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
    msg.exec();

    return std::string();
}


void snap_builder::on_meld_clicked()
{
    std::string const selection(get_selection_with_path());
    if(selection.empty())
    {
        return;
    }

    statusbar->showMessage("Compare changes with meld...");

    std::string cmd("cd ");
    cmd += selection;
    cmd += " && meld .";
    int const r(system(cmd.c_str()));
    if(r != 0)
    {
        QMessageBox msg(
              QMessageBox::Critical
            , "Meld Failed"
            , "Meld \""
                + QString::fromUtf8(cmd.c_str())
                + "\" failed."
            , QMessageBox::Close
            , this
            , Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
        msg.exec();
    }
    else
    {
        read_list_of_projects();
    }

    statusbar->clearMessage();
}


void snap_builder::on_edit_changelog_clicked()
{
    std::string const selection(get_selection_with_path());
    if(selection.empty())
    {
        return;
    }

    statusbar->showMessage("Editing changelog file...");

    std::string cmd("gvim --nofork ");
    cmd += selection;
    cmd += "/debian/changelog";
    int const r(system(cmd.c_str()));
    if(r != 0)
    {
        QMessageBox msg(
              QMessageBox::Critical
            , "Edit Command Failed"
            , "Edit command \""
                + QString::fromUtf8(cmd.c_str())
                + "\" failed."
            , QMessageBox::Close
            , this
            , Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
        msg.exec();
    }
    else
    {
        read_list_of_projects();
    }

    statusbar->clearMessage();
}


void snap_builder::on_bump_version_clicked()
{
    std::string const selection(get_selection_with_path());
    if(selection.empty())
    {
        return;
    }

    statusbar->showMessage("Increasing build version by 1...");

    std::string const version(f_current_project->get_version());
    advgetopt::string_list_t numbers;
    advgetopt::split_string(version, numbers, {"."});
    switch(numbers.size())
    {
    case 0:
        {
            QMessageBox msg(
                  QMessageBox::Critical
                , "Undefined Version"
                , "The version could not be determined for this project \""
                    + QString::fromUtf8(version.c_str())
                    + "\"."
                , QMessageBox::Close
                , this
                , Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
            msg.exec();
        }
        return;

    case 1:
        numbers.push_back("0");
#if __cplusplus >= 201700
        [[fallthrough]];
#endif
    case 2:
        numbers.push_back("0");
#if __cplusplus >= 201700
        [[fallthrough]];
#endif
    case 3:
        numbers.push_back("1");
        break;

    default:
        numbers[3] = std::to_string(std::stoi(numbers[3]) + 1);
        break;

    }

    std::string const new_version(
              numbers[0]
            + '.'
            + numbers[1]
            + '.'
            + numbers[2]
            + '.'
            + numbers[3]);

    std::string cmd("cd ");
    cmd += selection;
    cmd += " && dch --newversion ";
    cmd += new_version;
    cmd += "~";
    cmd += f_distribution;
    cmd += " --urgency high --distribution ";
    cmd += f_distribution;
    cmd += " \"Bumped build version to rebuild on Launchpad.\"";
    int r(system(cmd.c_str()));
    if(r != 0)
    {
        QMessageBox msg(
              QMessageBox::Critical
            , "Bump Version Failed"
            , "Increasing version to \""
                + QString::fromUtf8(new_version.c_str())
                + "\" failed."
            , QMessageBox::Close
            , this
            , Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
        msg.exec();
    }
    else
    {
        bool refresh_status(true);

        // I don't think that testing the state makes sense herel
        // if we had the right to bump the version, we should have the
        // right to commit + push automatically
        //
        //if(f_current_project->get_state() == "ready")
        {
            QMessageBox::StandardButton const result(QMessageBox::question(
                  this
                , "Bump Version Success"
                , "Do you want to auto-commit/push?"));
            if(result == QMessageBox::Yes)
            {
                // commit
                //
                std::string cmd_commit("cd ");
                cmd_commit += selection;
                cmd_commit += " && git commit -m \"Bumped build version to rebuild on Launchpad.\" debian/changelog";
                r = system(cmd_commit.c_str());
                if(r != 0)
                {
                    QMessageBox msg(
                          QMessageBox::Critical
                        , "Commit Failed"
                        , "The git command \""
                            + QString::fromUtf8(cmd_commit.c_str())
                            + "\" failed. See your console for details."
                        , QMessageBox::Close
                        , this
                        , Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
                    msg.exec();
                }
                else
                {
                    refresh_status = false;

                    read_list_of_projects();

                    if(f_current_project->get_state() == "not pushed")
                    {
                        // push
                        //
                        on_git_push_clicked();
                    }
                }
            }
        }
        if(refresh_status)
        {
            read_list_of_projects();
        }
    }

    statusbar->clearMessage();
}


void snap_builder::on_edit_control_clicked()
{
    std::string const selection(get_selection_with_path());
    if(selection.empty())
    {
        return;
    }

    statusbar->showMessage("Editing control file...");

    std::string cmd("gvim --nofork ");
    cmd += selection;
    cmd += "/debian/control";
    int const r(system(cmd.c_str()));
    if(r != 0)
    {
        QMessageBox msg(
              QMessageBox::Critical
            , "Edit Command Failed"
            , "Edit command \""
                + QString::fromUtf8(cmd.c_str())
                + "\" failed."
            , QMessageBox::Close
            , this
            , Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
        msg.exec();
    }

    statusbar->clearMessage();
}


void snap_builder::on_local_compile_clicked()
{
    std::string const selection(get_selection_with_path());
    if(selection.empty())
    {
        return;
    }

    statusbar->showMessage("Running local build of Release version...");

    std::string cmd("cd ");
    cmd += selection;
    cmd += " && ./mk -r -i";
    int const r(system(cmd.c_str()));
    if(r != 0)
    {
        QMessageBox msg(
              QMessageBox::Critical
            , "Local Compile Failed"
            , "The ./mk command \""
                + QString::fromUtf8(cmd.c_str())
                + "\" failed. See your console for details."
            , QMessageBox::Close
            , this
            , Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
        msg.exec();
    }

    statusbar->clearMessage();
}


void snap_builder::on_run_tests_clicked()
{
    std::string const selection(get_selection_with_path());
    if(selection.empty())
    {
        return;
    }

    statusbar->showMessage("Running tests locally...");

    std::string cmd("cd ");
    cmd += selection;
    cmd += " && ./mk -t";
    int const r(system(cmd.c_str()));
    if(r != 0)
    {
        QMessageBox msg(
              QMessageBox::Critical
            , "Tests Failed"
            , "The ./mk command \""
                + QString::fromUtf8(cmd.c_str())
                + "\" failed. See your console for details."
            , QMessageBox::Close
            , this
            , Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
        msg.exec();
    }

    statusbar->clearMessage();
}


void snap_builder::on_git_commit_clicked()
{
    std::string const selection(get_selection_with_path());
    if(selection.empty())
    {
        return;
    }

    std::string cmd("cd ");
    cmd += selection;
    cmd += " && GIT_EDITOR=\"gvim --nofork\" git commit .";
    int const r(system(cmd.c_str()));
    if(r != 0)
    {
        QMessageBox msg(
              QMessageBox::Critical
            , "Commit Failed"
            , "The git command \""
                + QString::fromUtf8(cmd.c_str())
                + "\" failed. See your console for details."
            , QMessageBox::Close
            , this
            , Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
        msg.exec();
    }
    else
    {
        read_list_of_projects();
    }
}


void snap_builder::on_git_push_clicked()
{
    std::string const selection(get_selection_with_path());
    if(selection.empty())
    {
        return;
    }

    std::string cmd("cd ");
    cmd += selection;
    cmd += " && git push";
    int const r(system(cmd.c_str()));
    if(r != 0)
    {
        QMessageBox msg(
              QMessageBox::Critical
            , "Push Failed"
            , "The git command \""
                + QString::fromUtf8(cmd.c_str())
                + "\" failed. See your console for details."
            , QMessageBox::Close
            , this
            , Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
        msg.exec();
    }
    else
    {
        read_list_of_projects();
    }
}


void snap_builder::on_git_pull_clicked()
{
    std::string const selection(get_selection_with_path());
    if(selection.empty())
    {
        return;
    }

    std::string cmd("cd ");
    cmd += selection;
    cmd += " && git pull";
    int const r(system(cmd.c_str()));
    if(r != 0)
    {
        QMessageBox msg(
              QMessageBox::Critical
            , "Pull Failed"
            , "The git command \""
                + QString::fromUtf8(cmd.c_str())
                + "\" failed. See your console for details."
            , QMessageBox::Close
            , this
            , Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
        msg.exec();
    }
    else
    {
        read_list_of_projects();
    }
}


void snap_builder::on_build_package_clicked()
{
    std::string const selection(get_selection());
    if(selection.empty())
    {
        return;
    }

    build_package->setStyleSheet("background-color: #e0a0a0;");
    build_package->setEnabled(false);
    build_package->setText("Building...");
    statusbar->showMessage("Send source to launchpad to build package...");

    std::cout << "\n----------------------------\nBuild package\n\n";

    std::string cmd(get_root_path());
    cmd += "/bin/send-to-launchpad.sh ";
    cmd += selection;
    int const r(system(cmd.c_str()));
    if(r != 0)
    {
        QMessageBox msg(
              QMessageBox::Critical
            , "Start of Build Failed"
            , "The send-to-launchpad command \""
                + QString::fromUtf8(cmd.c_str())
                + "\" failed. See your console for details."
            , QMessageBox::Close
            , this
            , Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
        msg.exec();
    }
    else
    {
        f_current_project->set_building(true);
    }

    statusbar->clearMessage();
    build_package->setText("Build Package");
    build_package->setEnabled(true);
    build_package->setStyleSheet(QString());
}


} // builder namespace
// vim: ts=4 sw=4 et
