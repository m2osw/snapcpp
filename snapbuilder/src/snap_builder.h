// Copyright (c) 2021-2022  Made to Order Software Corp.  All Rights Reserved
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
#include    "ui_snap_builder-MainWindow.h"
#include    "project.h"


// eventdispatcher lib
//
#include    <eventdispatcher/communicator.h>
#include    <eventdispatcher/qt_connection.h>


// advgetopt lib
//
#include    <advgetopt/advgetopt.h>


// snapdev lib
//
#include    <snapdev/not_reached.h>


// Qt includes
//
#include    <QCloseEvent>
#include    <QSettings>



namespace builder
{






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

protected:
    virtual void                    closeEvent(QCloseEvent * event) override;
    virtual void                    timerEvent(QTimerEvent *event) override;

private slots:
    void                            on_refresh_list_triggered();
    void                            on_refresh_clicked();
    void                            on_coverage_clicked();
    void                            on_build_release_triggered();
    void                            on_build_debug_triggered();
    void                            on_build_sanitize_triggered();
    void                            on_generate_dependency_svg_triggered();
    void                            on_mark_build_done_triggered();
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
    void                            adjust_columns();
    std::string                     get_selection() const;
    std::string                     get_selection_with_path() const;
    void                            set_button_status();
    bool                            svg_ready(
                                          cppprocess::io * output_pipe
                                        , cppprocess::done_reason_t reason);
    void                            update_state(int row);

    QSettings                       f_settings = QSettings();
    advgetopt::getopt               f_opt;
    ed::communicator::pointer_t     f_communicator = ed::communicator::pointer_t();
    ed::qt_connection::pointer_t    f_qt_connection = ed::qt_connection::pointer_t();
    std::string                     f_root_path = std::string();
    std::string                     f_config_path = std::string();
    std::string                     f_cache_path = std::string();
    std::string                     f_launchpad_url = std::string("https://api.launchpad.net/devel/~snapcpp/+archive/ubuntu/ppa?ws.op=getBuildRecords&ws.size=10&ws.start=0&source_name=@PROJECT_NAME@");
    std::string                     f_distribution = std::string("bionic");
    project::vector_t               f_projects = project::vector_t();
    project::pointer_t              f_current_project = project::pointer_t();
    advgetopt::string_list_t        f_release_names = advgetopt::string_list_t();
    int                             f_timer_id = 0;
};
//#pragma GCC diagnostic pop


} // builder namespace
// vim: ts=4 sw=4 et
