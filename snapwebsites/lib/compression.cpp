// Snap Websites Server -- compress (decompress) data
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

#include "compression.h"
#include <zlib.h>
#include <QMap>

namespace snap
{
namespace compression
{

namespace
{
typedef QMap<QString, compressor_t *>   compressor_map_t;

// IMPORTANT NOTE:
// This list only makes use of bare pointer for many good reasons.
// (i.e. all compressors are defined statitcally)
// Do not try to change it! Thank you.
compressor_map_t *g_compressors;

int bound_level(int level, int min, int max)
{
    return level < min ? min : (level > max ? max : level);
}

}


/** \brief Special compressor name to get the best compression available.
 *
 * Whenever we send a page on the Internet, we can compress it with zlib
 * (gzip, really). However, more and more, browsers are starting to support
 * other compressors. For example, Chrome supports "sdch" (a vcdiff
 * compressor) and FireFox is testing with lzma.
 *
 * Using the name "best" for the compressor will test with all available
 * compressions and return the smallest result whatever it is.
 */
const char *compressor_t::BEST_COMPRESSION = "best";


/** \brief Special compressor name returned in some cases.
 *
 * When trying to compress a buffer, there are several reasons why the
 * compression may "fail". When that happens the result is the same
 * as the input, meaning that the data is not going to be compressed
 * at all.
 *
 * You should always verify whether the compression worked by testing
 * the compressor_name variable on return.
 */
const char *compressor_t::NO_COMPRESSION = "none";


/** \brief Register the compressor.
 *
 * Whenever you implement a compressor, the constructor must call
 * this constructor with the name of the compressor. Remember that
 * the get_name() function is NOT ready from this constructor which
 * is why we require you to specify the name in the constructor.
 *
 * This function registers the compressor in the internal list of
 * compressors and then returns.
 */
compressor_t::compressor_t(const char *name)
{
    if(g_compressors == NULL)
    {
        g_compressors = new compressor_map_t;
    }
    (*g_compressors)[name] = this;
}


/** \brief Clean up the compressor.
 *
 * This function unregisters the compressor. Note that it is expected
 * that compressors get destroyed on exit only as they are expected
 * to be implemented and defined statically.
 */
compressor_t::~compressor_t()
{
    // TBD we probably don't need this code...
    //     it is rather slow so why waste our time on exit?
    //for(compressor_map_t::iterator
    //        it(g_compressors.begin());
    //        it != g_compressors.end();
    //        ++it)
    //{
    //    if(*it == this)
    //    {
    //        g_compressors.erase(it);
    //        break;
    //    }
    //}
}


/** \brief Return a list of available compressors.
 *
 * In case you have more than one Accept-Encoding this list may end up being
 * helpful to know whether a compression is available or not.
 *
 * \return A list of all the available compressors.
 */
QStringList compressor_list()
{
    QStringList list;
    for(compressor_map_t::const_iterator
            it(g_compressors->begin());
            it != g_compressors->end();
            ++it)
    {
        list.push_back((*it)->get_name());
    }
    return list;
}


/** \brief Compress the input buffer.
 *
 * This function compresses the input buffer and returns the result
 * in a copy.
 *
 * IMPORTANT NOTE:
 *
 * There are several reasons why the compressor may refuse compressing
 * your input buffer and return the input as is. When this happens the
 * name of the compressor is changed to NO_COMPRESSION.
 *
 * \li The input is empty.
 * \li The input buffer is too small for that compressor.
 * \li The level is set to a value under 5%.
 * \li The buffer is way too large and allocating the compression buffer
 *     failed (this should never happen on a serious server!)
 * \li The named compressor does not exist.
 *
 * Again, if the compression fails for whatever reason, the compressor_name
 * is set to NO_COMPRESSION. You have to make sure to test that name on
 * return to know what worked and what failed.
 *
 * \param[in,out] compressor_name  The name of the compressor to use.
 * \param[in] input  The input buffer which has to be compressed.
 * \param[in] level  The level of compression (0 to 100).
 * \param[in] text  Whether the input is text, set to false if not sure.
 *
 * \return A byte array with the compressed input data.
 */
QByteArray compress(QString& compressor_name, const QByteArray& input, level_t level, bool text)
{
    // nothing to compress if empty
    if(input.size() == 0 || level < 5)
    {
#ifdef DEBUG
printf("nothing to compress\n");
#endif
        compressor_name = compressor_t::NO_COMPRESSION;
        return input;
    }

    if(compressor_name == compressor_t::BEST_COMPRESSION)
    {
        QByteArray best;
        for(compressor_map_t::const_iterator
                it(g_compressors->begin());
                it != g_compressors->end();
                ++it)
        {
            if(best.size() == 0)
            {
                best = (*it)->compress(input, level, text);
                compressor_name = (*it)->get_name();
            }
            else
            {
                QByteArray test((*it)->compress(input, level, text));
                if(test.size() < best.size())
                {
                    best = test;
                    compressor_name = (*it)->get_name();
                }
            }
        }
        return best;
    }

    if(!g_compressors->contains(compressor_name))
    {
        // compressor is not available, return input as is...
        compressor_name = compressor_t::NO_COMPRESSION;
#ifdef DEBUG
printf("compressor not found?!\n");
#endif
        return input;
    }

    // avoid the compression if the result is larger or equal to the input!
    QByteArray result((*g_compressors)[compressor_name]->compress(input, level, text));
    if(result.size() >= input.size())
    {
        compressor_name = compressor_t::NO_COMPRESSION;
#ifdef DEBUG
printf("compression is larger?!\n");
#endif
        return input;
    }
    return result;
}


/** \brief Decompress a buffer.
 *
 * This function checks the specified input buffer and decompresses it if
 * a compressor recognized its magic signature.
 *
 * If none of the compressors were compatible then the input is returned
 * as is. The compressor_name is set to NO_COMPRESSION in this case. This
 * does not really mean the buffer is not compressed, although it is likely
 * correct.
 *
 * \param[out] compressor_name  Receives the name of the compressor used
 *                              to decompress the input data.
 * \param[in] input  The input to decompress.
 *
 * \return The decompressed buffer.
 */
QByteArray decompress(QString& compressor_name, const QByteArray& input)
{
    // nothing to decompress if empty
    if(input.size() == 0)
    {
        return input;
    }

    for(compressor_map_t::const_iterator
            it(g_compressors->begin());
            it != g_compressors->end();
            ++it)
    {
        if((*it)->compatible(input))
        {
            return (*it)->decompress(input);
        }
    }
    compressor_name = compressor_t::NO_COMPRESSION;
    return input;
}


class gzip_t : public compressor_t
{
public:
    gzip_t() : compressor_t("gzip")
    {
    }

    virtual const char *get_name() const
    {
        return "gzip";
    }

    virtual QByteArray compress(const QByteArray& input, level_t level, bool text)
    {
        // transform the 0 to 100 level to the standard 1 to 9 in zlib
        int zlib_level(bound_level((level * 2 + 25) / 25, Z_BEST_SPEED, Z_BEST_COMPRESSION));
        // initialize the zlib stream
        z_stream strm;
        memset(&strm, 0, sizeof(strm));
        // deflateInit2 expects the input to be defined
        strm.avail_in = input.size();
        strm.next_in = const_cast<Bytef *>(reinterpret_cast<const Bytef *>(input.data()));
        int ret(deflateInit2(&strm, zlib_level, Z_DEFLATED, 15 + 16, 9, Z_DEFAULT_STRATEGY));
        if(ret != Z_OK)
        {
            // compression failed, return input as is
            return input;
        }

        // initialize the gzip header
        gz_header header;
        memset(&header, 0, sizeof(header));
        header.text = text;
        header.time = time(NULL);
        header.os = 3;
        header.comment = const_cast<Bytef *>(reinterpret_cast<const Bytef *>("Snap! Websites"));
        //header.hcrc = 1; -- would that be useful?
        ret = deflateSetHeader(&strm, &header);
        if(ret != Z_OK)
        {
            deflateEnd(&strm);
            return input;
        }

        // prepare to call the deflate function
        // (to do it in one go!)
        // TODO check the size of the input buffer, if really large
        //      (256Kb?) then break this up in multiple iterations
        QByteArray result;
        result.resize(deflateBound(&strm, strm.avail_in));
        strm.avail_out = result.size();
        strm.next_out = reinterpret_cast<Bytef *>(result.data());

        // compress in one go
        ret = deflate(&strm, Z_FINISH);
        if(ret != Z_STREAM_END)
        {
            deflateEnd(&strm);
            return input;
        }
        // lose the extra size returned by deflateBound()
        result.resize(result.size() - strm.avail_out);
        deflateEnd(&strm);
        return result;
    }

    virtual bool compatible(const QByteArray& input) const
    {
        // the header is at least 10 bytes
        // the magic code (identification) is 0x1F 0x8B
        return input.size() >= 10 && input[0] == 0x1F && input[1] == static_cast<char>(0x8B);
    }

    virtual QByteArray decompress(const QByteArray& input)
    {
        // initialize the zlib stream
        z_stream strm;
        memset(&strm, 0, sizeof(strm));
        // inflateInit2 expects the input to be defined
        strm.avail_in = input.size();
        strm.next_in = const_cast<Bytef *>(reinterpret_cast<const Bytef *>(input.data()));
        int ret(inflateInit2(&strm, 15 + 16));
        if(ret != Z_OK)
        {
            // compression failed, return input as is
            return input;
        }

        // Unfortunately the zlib support for the gzip header doesn't help
        // us getting the ISIZE which is saved as the last 4 bytes of the
        // files (frankly?!)
        //
        // initialize the gzip header
        //gz_header header;
        //memset(&header, 0, sizeof(header));
        //ret = inflateGetHeader(&strm, &header);
        //if(ret != Z_OK)
        //{
        //    inflateEnd(&strm);
        //    return input;
        //}
        // The size is saved in the last 4 bytes in little endian
        size_t uncompressed_size(input[strm.avail_in - 4]
                | (input[strm.avail_in - 3] << 8)
                | (input[strm.avail_in - 2] << 16)
                | (input[strm.avail_in - 1] << 24));

        // prepare to call the deflate function
        // (to do it in one go!)
        // TODO check the size of the input buffer, if really large
        //      (256Kb?) then break this up in multiple iterations
        QByteArray result;
        result.resize(uncompressed_size);
        strm.avail_out = result.size();
        strm.next_out = reinterpret_cast<Bytef *>(result.data());

        // decompress in one go
        ret = inflate(&strm, Z_FINISH);
        if(ret != Z_STREAM_END)
        {
            inflateEnd(&strm);
            return input;
        }
        inflateEnd(&strm);
        return result;
    }

} gzip; // create statically


class deflate_t : public compressor_t
{
public:
    deflate_t() : compressor_t("deflate")
    {
    }

    virtual const char *get_name() const
    {
        return "deflate";
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
    virtual QByteArray compress(const QByteArray& input, level_t level, bool text)
    {
        // transform the 0 to 100 level to the standard 1 to 9 in zlib
        int zlib_level(bound_level((level * 2 + 25) / 25, Z_BEST_SPEED, Z_BEST_COMPRESSION));
        // initialize the zlib stream
        z_stream strm;
        memset(&strm, 0, sizeof(strm));
        // deflateInit2 expects the input to be defined
        strm.avail_in = input.size();
        strm.next_in = const_cast<Bytef *>(reinterpret_cast<const Bytef *>(input.data()));
        int ret(deflateInit2(&strm, zlib_level, Z_DEFLATED, 15, 9, Z_DEFAULT_STRATEGY));
        if(ret != Z_OK)
        {
            // compression failed, return input as is
            return input;
        }

        // prepare to call the deflate function
        // (to do it in one go!)
        // TODO check the size of the input buffer, if really large
        //      (256Kb?) then break this up in multiple iterations
        QByteArray result;
        result.resize(deflateBound(&strm, strm.avail_in));
        strm.avail_out = result.size();
        strm.next_out = reinterpret_cast<Bytef *>(result.data());

        // compress in one go
        ret = deflate(&strm, Z_FINISH);
        if(ret != Z_STREAM_END)
        {
            deflateEnd(&strm);
            return input;
        }
        // lose the extra size returned by deflateBound()
        result.resize(result.size() - strm.avail_out);
        deflateEnd(&strm);
        return result;
    }

    virtual bool compatible(const QByteArray& input) const
    {
        // there is no magic header in this one...
        return false;
    }

    virtual QByteArray decompress(const QByteArray& input)
    {
        throw std::runtime_error("the deflate decompress() function is not yet implemented, mainly because it is not accessible via the compression::decompress() function.");
    }
#pragma GCC pop

} deflate; // create statically


} // namespace snap
} // namespace compression
// vim: ts=4 sw=4 et
