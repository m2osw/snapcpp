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
#include    "project.h"
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
    f_table->setSelectionBehavior(QTableWidget::SelectRows);
    f_table->setSelectionMode(QTableWidget::SingleSelection);

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

    std::string reselect;
    if(f_current_project != nullptr)
    {
        reselect = f_current_project->get_name();
        f_current_project.reset();
    }

    f_projects.clear();

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

    f_table->clearContents();   // restart from scratch

    int const count(std::count_if(
                  f_projects.begin()
                , f_projects.end()
                , [](project::pointer_t p) { return p->is_valid(); }));
    f_table->setRowCount(count);

    QTableWidgetItem  * item = nullptr;
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

            item = new QTableWidgetItem("-"); // TODO -- launchpad version
            item->setData(Qt::UserRole, v);
            f_table->setItem(row, 2, item);

            item = new QTableWidgetItem(QString::fromUtf8(p->get_state().c_str()));
            item->setData(Qt::UserRole, v);
            f_table->setItem(row, 3, item);

            item = new QTableWidgetItem(QString::fromUtf8(p->get_last_commit_as_string().c_str()));
            item->setData(Qt::UserRole, v);
            f_table->setItem(row, 4, item);

            item = new QTableWidgetItem("-"); // TODO -- compiled date (on launchpad)
            item->setData(Qt::UserRole, v);
            f_table->setItem(row, 5, item);

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


void snap_builder::on_build_release_triggered()
{
    statusbar->showMessage("Build Release version of the entire Snap! C++ environment...");

    // TODO: make it output in a Qt window and prevent doubling the call...
    //
    std::string cmd("make -C ");
    cmd += get_root_path();
    cmd += "/BUILD/Release &";
    std::cout << "\n-----------------------------------------\ncommand: " << cmd << "\n";
    system(cmd.c_str());

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
    system(cmd.c_str());

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
    system(cmd.c_str());

    statusbar->clearMessage();
}


void snap_builder::on_action_quit_triggered()
{
    close();
}


void snap_builder::on_f_table_clicked(QModelIndex const & index)
{
    QList<QTableWidgetItem *> items(f_table->selectedItems());
    if(items.empty())
    {
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
        edit_changelog->setEnabled(false);
        edit_control->setEnabled(false);
        bump_version->setEnabled(false);
        local_compile->setEnabled(false);
        git_commit->setEnabled(false);
        git_push->setEnabled(false);
        git_pull->setEnabled(false);
    }
    else
    {
        // TODO: test everything necessary to properly set the status of the
        //       buttons and not just all enabled...
        //       (i.e. the version/state on Launchpad are important)

        f_current_selection->setText(QString::fromUtf8(f_current_project->get_name().c_str()));

        std::string const & state(f_current_project->get_state());
        build_package->setEnabled(state == "ready");
        edit_changelog->setEnabled(true);
        bump_version->setEnabled(true);
        edit_control->setEnabled(true);
        local_compile->setEnabled(true);
        git_commit->setEnabled(state == "not committed");
        git_push->setEnabled(state == "not pushed");
        git_pull->setEnabled(state == "ready");
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

    QMessageBox(
          QMessageBox::Critical
        , "Project Directory Not Found"
        , QString("We could not find the directory for project \"")
            + QString::fromUtf8(path.c_str())
            + "\""
        , QMessageBox::Close
        , const_cast<snap_builder *>(this)
        , Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);

    return std::string();
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
        QMessageBox(
              QMessageBox::Critical
            , "Edit Command Failed"
            , "Edit command \""
                + QString::fromUtf8(cmd.c_str())
                + "\" failed."
            , QMessageBox::Close
            , this
            , Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
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
        QMessageBox(
              QMessageBox::Critical
            , "Undefined Version"
            , "The version could not be determined for this project \""
                + QString::fromUtf8(version.c_str())
                + "\"."
            , QMessageBox::Close
            , this
            , Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
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
        QMessageBox(
              QMessageBox::Critical
            , "Bump Version Failed"
            , "Increasing version to \""
                + QString::fromUtf8(new_version.c_str())
                + "\" failed."
            , QMessageBox::Close
            , this
            , Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
    }
    else
    {
        bool refresh_status(true);
        if(f_current_project->get_state() == "ready")
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
                    QMessageBox(
                          QMessageBox::Critical
                        , "Commit Failed"
                        , "The git command \""
                            + QString::fromUtf8(cmd_commit.c_str())
                            + "\" failed. See your console for details."
                        , QMessageBox::Close
                        , this
                        , Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
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
        QMessageBox(
              QMessageBox::Critical
            , "Edit Command Failed"
            , "Edit command \""
                + QString::fromUtf8(cmd.c_str())
                + "\" failed."
            , QMessageBox::Close
            , this
            , Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
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
        QMessageBox(
              QMessageBox::Critical
            , "Local Compile Failed"
            , "The ./mk command \""
                + QString::fromUtf8(cmd.c_str())
                + "\" failed. See your console for details."
            , QMessageBox::Close
            , this
            , Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
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
        QMessageBox(
              QMessageBox::Critical
            , "Commit Failed"
            , "The git command \""
                + QString::fromUtf8(cmd.c_str())
                + "\" failed. See your console for details."
            , QMessageBox::Close
            , this
            , Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
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
        QMessageBox(
              QMessageBox::Critical
            , "Push Failed"
            , "The git command \""
                + QString::fromUtf8(cmd.c_str())
                + "\" failed. See your console for details."
            , QMessageBox::Close
            , this
            , Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
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
        QMessageBox(
              QMessageBox::Critical
            , "Pull Failed"
            , "The git command \""
                + QString::fromUtf8(cmd.c_str())
                + "\" failed. See your console for details."
            , QMessageBox::Close
            , this
            , Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
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

    statusbar->showMessage("Send source to launchpad to build package...");

    std::string cmd(get_root_path());
    cmd += "/bin/send-to-launchpad.sh ";
    cmd += selection;
    int const r(system(cmd.c_str()));
    if(r != 0)
    {
        QMessageBox(
              QMessageBox::Critical
            , "Start of Build Failed"
            , "The send-to-launchpad command \""
                + QString::fromUtf8(cmd.c_str())
                + "\" failed. See your console for details."
            , QMessageBox::Close
            , this
            , Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
    }

    statusbar->clearMessage();
}


} // builder namespace
// vim: ts=4 sw=4 et
