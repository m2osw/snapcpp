// Copyright (c) 2013-2022  Made to Order Software Corp.  All Rights Reserved.
//
// https://snapwebsites.org/project/sitter
// contact@m2osw.com
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
// Snap Websites Server -- snapwebsites flag functionality
#pragma once

// libexcept
//
#include    <libexcept/exception.h>


// C++
//
//#include    <list>
#include    <memory>
#include    <set>


namespace sitter
{

DECLARE_MAIN_EXCEPTION(flags_exception);

DECLARE_EXCEPTION(flags_exception, flags_exception_invalid_parameter);
DECLARE_EXCEPTION(flags_exception, flags_exception_invalid_name);
DECLARE_EXCEPTION(flags_exception, flags_exception_too_many_flags);



class flag
{
public:
    typedef std::shared_ptr<flag>               pointer_t;
    typedef std::list<pointer_t>                list_t;

    typedef std::set<std::string>               tag_list_t;

    static constexpr std::size_t                FLAGS_LIMIT = 100;

    enum class state_t
    {
        STATE_UP,       // something is in error
        STATE_DOWN      // delete error file
    };

                                flag(std::string const & unit, std::string const & section, std::string const & name);
                                flag(std::string const & filename);

    flag &                      set_state(state_t state);
    flag &                      set_source_file(std::string const & source_file);
    flag &                      set_function(std::string const & function);
    flag &                      set_line(int line);
    flag &                      set_message(std::string const & message);
    flag &                      set_priority(int priority);
    flag &                      set_manual_down(bool manual);
    flag &                      add_tag(std::string const & tag);

    state_t                     get_state() const;
    std::string const &         get_unit() const;
    std::string const &         get_section() const;
    std::string const &         get_name() const;
    std::string const &         get_filename() const;
    std::string const &         get_source_file() const;
    std::string const &         get_function() const;
    int                         get_line() const;
    std::string const &         get_message() const;
    int                         get_priority() const;
    bool                        get_manual_down() const;
    time_t                      get_date() const;
    time_t                      get_modified() const;
    tag_list_t const &          get_tags() const;
    std::string const &         get_hostname() const;
    int                         get_count() const;
    std::string const &         get_version() const;

    bool                        save();

    static list_t               load_flags();

private:
    static void                 valid_name(std::string & name);

    state_t                     f_state             = state_t::STATE_UP;
    std::string                 f_unit              = std::string();
    std::string                 f_section           = std::string();
    std::string                 f_name              = std::string();
    mutable std::string         f_filename          = std::string();
    std::string                 f_source_file       = std::string();
    std::string                 f_function          = std::string();
    int                         f_line              = 0;
    std::string                 f_message           = std::string();
    int                         f_priority          = 5;
    bool                        f_manual_down       = false;
    time_t                      f_date              = -1;
    time_t                      f_modified          = -1;
    tag_list_t                  f_tags              = tag_list_t();
    std::string                 f_hostname          = std::string();
    int                         f_count             = 0;
    std::string                 f_version           = std::string();
};




#define SITTER_FLAG_UP(unit, section, name, message)   \
            std::make_shared<sitter::flag>( \
                sitter::flag(unit, section, name) \
                    .set_message(message) \
                    .set_source_file(__FILE__) \
                    .set_function(__func__) \
                    .set_line(__LINE__))

#define SITTER_FLAG_DOWN(unit, section, name) \
            std::make_shared<sitter::flag>( \
                sitter::flag(unit, section, name) \
                    .set_state(sitter::flag::state_t::STATE_DOWN) \
                    .set_source_file(__FILE__) \
                    .set_function(__func__) \
                    .set_line(__LINE__))



} // namespace sitter
// vim: ts=4 sw=4 et
