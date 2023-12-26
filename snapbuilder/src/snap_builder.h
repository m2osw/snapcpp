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
#pragma once

// self
//
#include    "background_processing.h"
#include    "ui_snap_builder-MainWindow.h"
#include    "project.h"


// eventdispatcher
//
#include    <eventdispatcher/communicator.h>
#include    <eventdispatcher/qt_connection.h>


// cppthread
//
#include    <cppthread/thread.h>


// advgetopt
//
#include    <advgetopt/advgetopt.h>


// snapdev
//
#include    <snapdev/lockfile.h>
#include    <snapdev/not_reached.h>


// Qt
//
#include    <QCloseEvent>
#include    <QSettings>



namespace builder
{



enum column_t : int
{
    COLUMN_PROJECT_NAME,
    COLUMN_CURRENT_VERSION,
    COLUMN_LAUNCHPAD_VERSION,
    COLUMN_CHANGES,
    COLUMN_LOCAL_CHANGES_DATE,
    COLUMN_BUILD_STATE,
    COLUMN_LAUNCHPAD_COMPILED_DATE,
};



// the main object, which is also a Qt window
//#pragma GCC diagnostic push
//#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
class snap_builder
    : public QMainWindow
    , private Ui::snap_builder
{
private:
    Q_OBJECT

public:
                                    snap_builder(int argc, char * argv[]);
                                    snap_builder(snap_builder const &) = delete;
    virtual                         ~snap_builder() override;

    snap_builder &                  operator = (snap_builder const &) = delete;

    void                            run();

    std::string const &             get_root_path() const;
    std::string const &             get_cache_path() const;
    std::string const &             get_flag_name(std::string const & project_name) const;
    std::string const &             get_launchpad_url() const;
    advgetopt::string_list_t const &get_release_names() const;

    void                            project_changed(project::pointer_t p);
    void                            process_git_push(project::pointer_t p);
    void                            adjust_columns();
    bool                            is_background_thread() const;

protected:
    virtual void                    closeEvent(QCloseEvent * event) override;
    //virtual void                    timerEvent(QTimerEvent *event) override;

signals:
    void                            projectChanged(project_ptr p);
    void                            adjustColumns();
    void                            gitPush(project_ptr p);

private slots:
    void                            on_project_changed(project_ptr p);
    void                            on_adjust_columns();
    void                            on_git_push(project_ptr p);
    void                            on_refresh_list_triggered();
    void                            on_refresh_project_triggered();
    void                            on_local_refresh_clicked();
    void                            on_remote_refresh_clicked();
    void                            on_coverage_clicked();
    void                            on_build_release_triggered();
    void                            on_build_debug_triggered();
    void                            on_build_sanitize_triggered();
    void                            on_generate_dependency_svg_triggered();
    void                            on_mark_build_done_triggered();
    void                            on_clear_launchpad_caches_triggered();
    void                            on_action_quit_triggered();
    void                            on_about_snapbuilder_triggered();
    void                            on_f_table_clicked(QModelIndex const & index);
    void                            on_meld_clicked();
    void                            on_edit_changelog_clicked();
    void                            on_bump_version_clicked();
    void                            on_edit_control_clicked();
    void                            on_local_compile_clicked();
    void                            on_run_tests_clicked();
    void                            on_git_commit_clicked();
    void                            on_git_push_clicked();
    void                            on_git_pull_clicked();
    void                            on_build_package_clicked();

private:
    void                            read_list_of_projects();
    std::string                     get_selection() const;
    std::string                     get_selection_with_path(std::string path = std::string()) const;
    void                            set_button_status();
    bool                            git_push_project(std::string const & selection);
    bool                            svg_ready(
                                          cppprocess::io * output_pipe
                                        , cppprocess::done_reason_t reason);
    void                            update_state(int row);
    int                             find_row(project::pointer_t p) const;

    QSettings                       f_settings = QSettings();
    advgetopt::getopt               f_opt;
    ed::communicator::pointer_t     f_communicator = ed::communicator::pointer_t();
    ed::qt_connection::pointer_t    f_qt_connection = ed::qt_connection::pointer_t();
    std::string                     f_root_path = std::string();
    std::string                     f_config_path = std::string();
    std::string                     f_cache_path = std::string();
    std::string                     f_launchpad_url = std::string();
    std::string                     f_distribution = std::string("bionic");
    project::vector_t               f_projects = project::vector_t();
    project::pointer_t              f_current_project = project::pointer_t();
    advgetopt::string_list_t        f_release_names = advgetopt::string_list_t();
    int                             f_timer_id = 0;
    std::shared_ptr<snapdev::lockfile>
                                    f_lockfile = std::shared_ptr<snapdev::lockfile>();
    bool                            f_auto_update_svg = false;
    background_worker::pointer_t    f_background_worker = background_worker::pointer_t();
    cppthread::thread::pointer_t    f_worker_thread = cppthread::thread::pointer_t();
};
//#pragma GCC diagnostic pop


} // builder namespace
// vim: ts=4 sw=4 et
