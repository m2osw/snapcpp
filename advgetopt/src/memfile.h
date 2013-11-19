/*    memfile.h -- handle files in memory
 *    Copyright (C) 2012-2013  Made to Order Software Corporation
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License along
 *    with this program; if not, write to the Free Software Foundation, Inc.,
 *    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *    Authors
 *    Alexis Wilke   alexis@m2osw.com
 */
#pragma once

/** \file
 * \brief Declarations of the memory file classes.
 *
 * This file includes all the declarations of the memory file class which is
 * used to read files from disk or via HTTP and write files to disk.
 */
#include    "md5.h"
#include    "wpkgar_block.h"
#include    "libdebpackages/wpkg_filename.h"


namespace memfile {

// generic memfile exception
class memfile_exception : public std::runtime_error
{
public:
    memfile_exception(const std::string& msg) : runtime_error(msg) {}
};

// problem with compatibility
class memfile_exception_compatibility : public memfile_exception
{
public:
    memfile_exception_compatibility(const std::string& msg) : memfile_exception(msg) {}
};

// problem with I/O
class memfile_exception_io : public memfile_exception
{
public:
    memfile_exception_io(const std::string& msg) : memfile_exception(msg) {}
};

// invalid parameter
class memfile_exception_parameter : public memfile_exception
{
public:
    memfile_exception_parameter(const std::string& msg) : memfile_exception(msg) {}
};

// trying to use a file that is still undefined
class memfile_exception_undefined : public memfile_exception
{
public:
    memfile_exception_undefined(const std::string& msg) : memfile_exception(msg) {}
};

// some input is invalid
class memfile_exception_invalid : public memfile_exception
{
public:
    memfile_exception_invalid(const std::string& msg) : memfile_exception(msg) {}
};

class memory_file
{
public:
    class file_info
    {
    public:
        enum field_name_t
        {
            field_name_package_name,
            field_name_filename,
            field_name_file_type,
            field_name_link,
            field_name_user,
            field_name_group,
            field_name_uid,
            field_name_gid,
            field_name_mode,
            field_name_size,
            field_name_mtime,
            field_name_ctime,
            field_name_atime,
            field_name_dev_major,
            field_name_dev_minor,
            field_name_raw_md5sum,
            field_name_original_compression,
            field_name_max
        };

        enum file_type_t
        {
            regular_file,
            hard_link,
            symbolic_link,
            character_special,
            block_special,
            directory,
            fifo,
            continuous,

            // the following 3 types are reserved for internal use only
            long_filename,
            long_symlink,
            pax_header
        };

        file_info();
        void reset();
        bool is_field_defined(field_name_t field) const;
        void set_field(field_name_t field);
        void reset_field(field_name_t field);

        wpkg_filename::uri_filename get_uri() const;
        std::string get_package_name() const;
        std::string get_filename() const;
        std::string get_basename() const;
        file_type_t get_file_type() const;
        std::string get_link() const;
        std::string get_user() const;
        std::string get_group() const;
        int get_uid() const;
        int get_gid() const;
        int get_mode() const;
        std::string get_mode_flags() const;
        int get_size() const;
        time_t get_mtime() const;
        time_t get_ctime() const;
        time_t get_atime() const;
        std::string get_date() const;
        int get_dev_major() const;
        int get_dev_minor() const;
        const md5::raw_md5sum& get_raw_md5sum() const;
        wpkgar::wpkgar_block_t::wpkgar_compression_t get_original_compression() const;

        void set_uri(const wpkg_filename::uri_filename& uri);
        void set_package_name(const std::string& package_name);
        void set_filename(const std::string& filename);
        void set_filename(const char *fn, int max_size);
        void set_file_type(file_type_t t);
        void set_link(const std::string& link);
        void set_link(const char *lnk, int max_size);
        void set_user(const std::string& owner);
        void set_user(const char *o, int max_size);
        void set_group(const std::string& group);
        void set_group(const char *g, int max_size);
        void set_uid(int uid);
        void set_uid(const char *u, int max_size, int base);
        void set_gid(int gid);
        void set_gid(const char *g, int max_size, int base);
        void set_mode(int mode);
        void set_mode(const char *m, int max_size, int base);
        void set_size(int size);
        void set_size(const char *s, int max_size, int base);
        void set_mtime(time_t mtime);
        void set_mtime(const char *t, int max_size, int base);
        void set_ctime(time_t ctime);
        void set_ctime(const char *t, int max_size, int base);
        void set_atime(time_t atime);
        void set_atime(const char *t, int max_size, int base);
        void set_dev_major(int dev);
        void set_dev_major(const char *d, int max_size, int base);
        void set_dev_minor(int dev);
        void set_dev_minor(const char *d, int max_size, int base);
        void set_raw_md5sum(md5::raw_md5sum& raw);
        void set_original_compression(wpkgar::wpkgar_block_t::wpkgar_compression_t original_compression);

        static int strnlen(const char *s, int n);
        static int str_to_int(const char *s, int n, int base);
        static void int_to_str(char *d, uint32_t value, int len, int base, char fill);

    private:
        wpkg_filename::uri_filename  f_uri;
        std::vector<bool>       f_defined;          // whether a field was set
        std::string             f_package_name;     // used in some circumstances, but not saved in archives
        std::string             f_filename;
        file_type_t             f_file_type;
        std::string             f_link;
        std::string             f_user;
        std::string             f_group;
        int                     f_uid;
        int                     f_gid;
        int                     f_mode;
        int                     f_size;
        time_t                  f_mtime;
        time_t                  f_atime;
        time_t                  f_ctime;
        int                     f_dev_major;
        int                     f_dev_minor;
        md5::raw_md5sum         f_raw_md5sum;
        wpkgar::wpkgar_block_t::wpkgar_compression_t f_original_compression;
    };

    enum file_format_t
    {
        // no file defined yet
        file_format_undefined,
        file_format_best, // best compression / archive

        // compressed files
        file_format_gz,
        file_format_bz2,
        file_format_lzma, // TODO...
        file_format_xz, // TODO...

        // archives
        file_format_directory, // hard drive directory
        file_format_ar,
        file_format_tar,
        file_format_zip, // TODO...
        file_format_7z, // TODO...
        file_format_wpkg, // for wpkg "database"
        file_format_meta, // the wpkg files meta data format (text files)

        // any other files
        file_format_other
    };

    class block_manager
    {
    public:
        // the block manager needs buffers that are at least 1Kb in size to
        // properly work with all the possible optimizations (i.e. 10 bits)
        static const int BLOCK_MANAGER_BUFFER_BITS = 16; // 16 bits represents buffers of 64Kb
        static const int BLOCK_MANAGER_BUFFER_SIZE = (1 << BLOCK_MANAGER_BUFFER_BITS);

        block_manager();
        ~block_manager();

        static int max_allocated(); // block_manager doesn't free anything

        void clear();
        int size() const { return f_size; }
        int read(char *buffer, int offset, int size) const;
        int write(const char *buffer, int offset, int size);
        int compare(const block_manager& rhs) const;

        file_format_t data_to_format(int offset, int size) const;

    private:
        typedef std::vector<char *>         buffer_t;

        static buffer_t                     g_free_buffers;
        static controlled_vars::zint32_t    g_total_allocated;

        controlled_vars::zint32_t           f_size;
        controlled_vars::zint32_t           f_available_size;
        buffer_t                            f_buffers;
    };

    static const int file_info_throw = 0x00;
    static const int file_info_return_errors = 0x01;
    static const int file_info_permissions_error = 0x02;
    static const int file_info_owner_error = 0x04;

    memory_file();

    // filename handling
    void set_filename(const wpkg_filename::uri_filename& filename);
    const wpkg_filename::uri_filename& get_filename() const;

    // basic format handling
    void guess_format_from_data();
    file_format_t get_format() const;
    bool is_text() const;
    static file_format_t data_to_format(const char *data, int size);
    static file_format_t filename_extension_to_format(const wpkg_filename::uri_filename& filename, bool ignore_compression = false);
    static std::string to_base64(const char *buf, size_t size);

    // read from and write to disk
    void read_file(const wpkg_filename::uri_filename& filename, file_info *info = NULL);
    void write_file(const wpkg_filename::uri_filename& filename, bool create_folders = false, bool force = false) const;
    void copy(memory_file& destination) const;
    int compare(const memory_file& rhs) const;

    // compression handling (gz or bz2)
    bool is_compressed() const;
    void compress(memory_file& result, file_format_t format, int zlevel = 9) const;
    void decompress(memory_file& result) const;

    // access the raw data
    void reset();
    void create(file_format_t format);
    void end_archive();
    int read(char *buffer, int offset, int bufsize) const;
    bool read_line(int& offset, std::string& result) const;
    int write(const char *buffer, const int offset, const int bufsize);
    void printf(const char *format, ...);
    void append_file(const file_info& info, const memory_file& data);
    int size() const;

    // access files in 'ar', 'tar', 'zip', '7z', or 'wpkgar' archives
    // as well as disk directories
    void dir_rewind(const wpkg_filename::uri_filename& path = wpkg_filename::uri_filename(), bool recursive = true);
    int dir_pos() const;
    bool dir_next(file_info& info, memory_file *data = NULL) const;
    int dir_size(const wpkg_filename::uri_filename& path, int& disk_size, int block_size = 512);
    void set_package_path(const wpkg_filename::uri_filename& path);
    static void disk_file_to_info(const wpkg_filename::uri_filename& filename, file_info& info);
    static void info_to_disk_file(const wpkg_filename::uri_filename& filename, const file_info& info, int& err);

    // compute md5sum of the entire file
    void raw_md5sum(md5::raw_md5sum& raw) const;
    std::string md5sum() const;

private:
    typedef controlled_vars::limited_auto_init<file_format_t, file_format_undefined, file_format_other, file_format_undefined>  safe_file_format_t;

    memory_file(const memory_file&);
    memory_file& operator = (memory_file&);
    void compress_to_gz(memory_file& result, int zlevel) const;
    void compress_to_bz2(memory_file& result, int zlevel) const;
    void decompress_from_gz(memory_file& result) const;
    void decompress_from_bz2(memory_file& result) const;
    bool dir_next_dir(file_info& info) const;
    void dir_next_ar(file_info& info) const;
    bool dir_next_tar(file_info& info) const;
    bool dir_next_tar_read(file_info& info) const;
    void dir_next_wpkg(file_info& info, memory_file *data) const;
    bool dir_next_meta(file_info& info) const;
    bool dir_next_sources(file_info& info) const;
    void append_ar(const file_info& info, const memory_file& data);
    void append_tar(const file_info& info, const memory_file& data);
    void append_tar_write(const file_info& info, const memory_file& data);
    void append_wpkg(const file_info& info, const memory_file& data);

    wpkg_filename::uri_filename                 f_filename;
    safe_file_format_t                          f_format;
    controlled_vars::fbool_t                    f_created;
    controlled_vars::fbool_t                    f_loaded;
    controlled_vars::fbool_t                    f_directory;
    controlled_vars::tbool_t                    f_recursive;
    controlled_vars::zint32_t                   f_dir_size;
    mutable std::shared_ptr<wpkg_filename::os_dir>   f_dir;
    mutable std::vector<std::shared_ptr<wpkg_filename::os_dir> >  f_dir_stack;
    mutable controlled_vars::zint32_t           f_dir_pos;
    block_manager                               f_buffer;
    wpkg_filename::uri_filename                 f_package_path;
};

} // namespace memfile

// vim: ts=4 sw=4 et
