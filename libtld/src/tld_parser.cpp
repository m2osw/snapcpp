// TLD library -- XML to C++ parser
// Copyright (C) 2011-2013  Made to Order Software Corp.
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

/** \file
 * \brief Parser of the tld_data.xml file.
 *
 * This file defines the parser of the XML data used to generate the
 * tld_data.c file.
 */

#include "libtld/tld.h"
#include <QtCore/QMap>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QStringList>
#include <QtXml/QDomDocument>
#include <iostream>
#include <cstdlib>

/** \brief [internal] Namespace used by the TLD parser.
 * \internal
 *
 * This namespace is used internally by the TLD parser too which loads the
 * XML data and transforms it to a .c file for the TLD library.
 */
namespace snap
{


/** \brief [internal] Class used to transform the XML data to TLD info structures.
 * \internal
 *
 * This class is used to read data from the XML data file and transform
 * that in TLD info structure in an optimized way to we can search the
 * data as quickly as possible.
 */
class tld_info
{
public:
    /// The category name to output for this TLD.
    QString                f_category;
    /// The reason name to output for this TLD.
    QString                f_reason;
    /// The category attribute of the area tag.
    QString                f_category_name;
    /// The country name for an area.
    QString                f_country;  // if category is "country", otherwise empty
    /// Level of this TLD.
    int                    f_level; // level of this TLD (1, 2, 3, 4)
    /// The complete TLD of this entry
    QString                f_tld;
    /// The inverted TLD to help us sort everything.
    QString                f_inverted;
    /// The reason attribute define in forbid tags.
    QString                f_reason_name;
    /// The TLD this exception applies to (i.e. the actual response)
    QString                f_exception_apply_to;
	/// The offset of this item in the final table.
    int                    f_offset;
	/// The start offset of a TLDs next level entries
    int                    f_start_offset;
	/// The end offset (excluded) of a TLDs next level entries
    int                    f_end_offset;
};

/// Type used to hold the list of all the info structures.
typedef QMap<QString, tld_info>    tld_info_map_t;

/// Type used to hold the list of all the countries.
typedef QMap<QString, int>    country_map_t;

/// Type used to hold all the TLDs by letters. We're actually not using that at this point.
typedef QMap<ushort, int>  tld_info_letters_t;


/// Encode a TLD so it gets sorted as expected.
QString tld_encode(const QString& tld, int& level)
{
    QString result;
    level = 0;

    QByteArray utf8 = tld.toUtf8();
    int max(utf8.length());
    const char *p = utf8.data();
    for(int l = 0; l < max; ++l) {
        char c(p[l]);
        if(static_cast<unsigned char>(c) < 0x20) {
            std::cerr << "error: controls characters (^" << (c + '@')
                    << ") are not allowed in TLDs ("
                    << p << ").\n";
            exit(1);
        }
        if((c >= 'A' && c <= 'Z')
        || (c >= 'a' && c <= 'z')
        || (c >= '0' && c <= '9')
        || c == '.' || c == '-')
        {
            // these are accepted as is; note that we already checked the
            // validty of the data w
            if(c == '.')
            {
                ++level;
                c = '!'; // this is important otherwise the sort can break
            }
            result += c;
        }
        else
        {
            // add/remove as appropriate
            if(c == '/' || c == ':' || c == '&') {
                std::cerr << "error: character (^" << c << ") is not allowed in TLDs.\n";
                exit(1);
            }
            result += '%';
            QString v(QString("%1").arg(c & 255, 2, 16, QLatin1Char('0')));
            result += v[0];
            result += v[1];
        }
    }
    // at this time the maximum level we declared is 4 but there are cases
    // where countries defined 5 levels (which is definitively crazy!)
    if(level < 1)
    {
        std::cerr << "error: level out of range (" << level << ") did you put a period at the beginning of the tld \"" << tld.toUtf8().data() << "\".\n";
        exit(1);
    }
    if(level > 5)
    {
        std::cerr << "error: level out of range (" << level << ") if larger than the maximum limit, you may want to increase the limit for \"" << tld.toUtf8().data() << "\".\n";
        exit(1);
    }

    // break it up to easily invert it
    QStringList split = result.split('!', QString::SkipEmptyParts);
    int i(0);
    int j(split.size() - 1);
    while(i < j) {
        split.swap(i, j);
        ++i;
        --j;
    }
    // save it back inverted (!a!b!c is now c!b!a!)
    result = split.join("!") + "!";

    return result;
}


/// Read data from the tld_data.xml file.
void read_tlds(const QString& path, tld_info_map_t& map, country_map_t& countries)
{
    // get input file
    QFile f(path + "/tld_data.xml");
    if(!f.open(QIODevice::ReadOnly)) {
        std::cerr << "error: cannot open " << path.toUtf8().data() << "/tld_data.xml input file\n";
        exit(1);
    }

    // create a DOM and attach file to it
    QDomDocument doc;
    doc.setContent(&f);

    // search for the tld tag
    QDomNode n = doc.firstChild();
    if(n.isNull()) {
        std::cerr << "error: your TLD document is empty.\n";
        exit(1);
    }
    while(!n.isNull()) {
        if(n.isElement()) {
            QDomElement tlc_tag = n.toElement();
            if(tlc_tag.tagName() != "tld") {
                std::cerr << "error: the root tag must be a <tld> tag. We got <" << tlc_tag.tagName().toUtf8().data() << "> instead.\n";
                exit(1);
            }
            break;
        }
        n = n.nextSibling();
    }
    if(n.isNull()) {
        std::cerr << "error: your TLD document is expected to have a <tld> tag as the root tag; we could not find it.\n";
        exit(1);
    }
    n = n.firstChild();

    int    country_counter = 0;

    // go through the <area> tags
    while(!n.isNull())
    {
        // make sure it's a tag
        if(n.isElement())
        {
            QDomElement e = n.toElement();
            if(e.tagName() != "area")
            {
                std::cerr << "error: only <area> tags are expected in a <tld> XML file, got <" << e.tagName().toUtf8().data() << "> instead.\n";
                exit(1);
            }

            // Category (international|professionals|language|groups|region|country)
            QString category(e.attribute("category", "country"));
            QString country;
            if(category == "country")
            {
                // Country Name
                country = e.attribute("country", "undefined");
                if(countries.contains(country))
                {
                    std::cerr << "error: found country \"" << country.toUtf8().data() << "\" defined twice.\n";
                    exit(1);
                }
                countries[country] = ++country_counter;
            }

            // Actual TLDs (may be empty)
            QDomNode t = e.firstChild();
            while(!t.isNull())
            {
                if(!t.isComment() && t.isCharacterData())
                {
                    QString names(t.toCharacterData().data());
                    names.replace("\n", " ");
                    names.replace("\r", " ");
                    names.replace("\t", " ");
                    QStringList name_list(names.split(" ", QString::SkipEmptyParts));
                    for(QStringList::iterator nm = name_list.begin();
                                              nm != name_list.end();
                                              ++nm)
                    {
                        if(nm->isEmpty())
                        {
                            continue;
                        }
                        int level(0);
                        QString value_name(tld_encode(*nm, level));
                        if(map.contains(value_name))
                        {
                            std::cerr << "error: found TLD \"" << nm->toUtf8().data() << "\" more than once.\n";
                            exit(1);
                        }

                        tld_info tld;
                        tld.f_category_name = category;
                        tld.f_country = country;
                        tld.f_level = level;
                        tld.f_tld = *nm;
                        tld.f_inverted = value_name;
                        // no reason, we're not inside a forbid tag
                        // no exception apply to, we're not inside an exception
                        tld.f_offset = 0;
                        tld.f_start_offset = USHRT_MAX;
                        tld.f_end_offset = USHRT_MAX;

                        map[value_name] = tld;
                    }
                }
                else if(t.isElement())
                {
                    QDomElement g = t.toElement();
                    if(g.tagName() == "exceptions")
                    {
                        QString apply_to(g.attribute("apply-to", "unknown"));
                        int unused_level(0);
                        apply_to = tld_encode(apply_to, unused_level);

                        QDomNode st = g.firstChild();
                        while(!st.isNull())
                        {
                            if(!st.isComment() && st.isCharacterData())
                            {
                                QString names(st.toCharacterData().data());
                                names.replace("\n", " ");
                                names.replace("\r", " ");
                                names.replace("\t", " ");
                                QStringList name_list(names.split(" ", QString::SkipEmptyParts));
                                for(QStringList::iterator nm = name_list.begin();
                                                          nm != name_list.end();
                                                          ++nm)
                                {
                                    int level(0);
                                    QString value_name(tld_encode(*nm, level));
                                    if(map.contains(value_name))
                                    {
                                        std::cerr << "error: found TLD \"" << nm->toUtf8().data() << "\" more than once (exceptions section).\n";
                                        exit(1);
                                    }

                                    tld_info tld;
                                    tld.f_category_name = category;
                                    tld.f_country = country;
                                    tld.f_level = level;
                                    tld.f_tld = *nm;
                                    tld.f_inverted = value_name;
                                    // no reason, we're not inside a forbid tag
                                    tld.f_exception_apply_to = apply_to;
                                    tld.f_offset = 0;
                                    tld.f_start_offset = USHRT_MAX;
                                    tld.f_end_offset = USHRT_MAX;

                                    map[value_name] = tld;
                                }
                            }
                            st = st.nextSibling();
                        }
                    }
                    else if(g.tagName() == "forbid")
                    {
                        QString reason(g.attribute("reason", "unused"));

                        QDomNode st = g.firstChild();
                        while(!st.isNull())
                        {
                            if(!st.isComment() && st.isCharacterData())
                            {
                                QString names(st.toCharacterData().data());
                                names.replace("\n", " ");
                                names.replace("\r", " ");
                                names.replace("\t", " ");
                                QStringList name_list(names.split(" ", QString::SkipEmptyParts));
                                for(QStringList::iterator nm = name_list.begin();
                                                          nm != name_list.end();
                                                          ++nm)
                                {
                                    int level(0);
                                    QString value_name(tld_encode(*nm, level));
                                    if(map.contains(value_name))
                                    {
                                        std::cerr << "error: found TLD \"" << nm->toUtf8().data() << "\" more than once (forbidden section).\n";
                                        exit(1);
                                    }

                                    tld_info tld;
                                    tld.f_category_name = category;
                                    tld.f_country = country;
                                    tld.f_level = level;
                                    tld.f_tld = *nm;
                                    tld.f_inverted = value_name;
                                    tld.f_reason_name = reason;
                                    // no exception apply to, we're not inside an exception
                                    tld.f_offset = 0;
                                    tld.f_start_offset = USHRT_MAX;
                                    tld.f_end_offset = USHRT_MAX;

                                    map[value_name] = tld;
                                }
                            }
                            st = st.nextSibling();
                        }
                    }
                    else {
                        std::cerr << "error: only <forbid> and <exceptions> tags are expected in an <area> tag, got <" << g.tagName().toUtf8().data() << "> instead.\n";
                        exit(1);
                    }
                }
                t = t.nextSibling();
            }
        }
        n = n.nextSibling();
    }
}


/// Verify the data we read from the tld_data.xml
void verify_data(tld_info_map_t& map)
{
    int max_tld_length = 0;
    for(tld_info_map_t::iterator it = map.begin();
                              it != map.end();
                              ++it)
    {
        QString t(it->f_tld);
        if(t.length() > max_tld_length)
        {
            max_tld_length = t.length();
        }
        for(int i = t.length() - 1, j = i + 1, k = j; i >= 0; --i)
        {
            QChar c = t.at(i);
            short u = c.unicode();
            if(u == '.')
            {
                // periods are accepted, but not one after another or just before a dash
                if(i + 1 == j)
                {
                    // this captures an ending period which we don't allow in our files (although it is legal in a domain name)
                    if(j == t.length())
                    {
                        std::cerr << "error: an ending period is not acceptable in a TLD name; found in \"" << t.toUtf8().data() << "\"\n";
                    }
                    else
                    {
                        std::cerr << "error: two periods one after another is not acceptable in a TLD name; found in \"" << t.toUtf8().data() << "\"\n";
                    }
                    exit(1);
                }
                if(i + 1 == k)
                {
                    std::cerr << "error: a dash cannot be just after a period; problem found in \"" << t.toUtf8().data() << "\"\n";
                    exit(1);
                }
                j = i;
                k = i;
            }
            else if(i == 0)
            {
                std::cerr << "error: the TLD must start with a period; problem found in \"" << t.toUtf8().data() << "\"\n";
                exit(1);
            }
            else if(u == '-')
            {
                if(i + 1 == k)
                {
                    if(k == t.length())
                    {
                        std::cerr << "error: a dash cannot be found at the end of a TLD; problem found in \"" << t.toUtf8().data() << "\"\n";
                    }
                    else
                    {
                        std::cerr << "error: a dash cannot be just before a period; problem found in \"" << t.toUtf8().data() << "\"\n";
                    }
                    exit(1);
                }
                k = i;
            }
            else if(!c.isLetterOrNumber())
            {
                // we accept a certain number of signs that are not
                // otherwise considered letters...
                switch(c.unicode()) {
                case 0x093E: // devanagari vowel sign AA
                case 0x0982: // Bengali Sign Anusvara
                case 0x09BE: // Bengali Vowel Sign AA
                case 0x0A3E: // Gurmukhi Vowel Sign AA
                case 0x0ABE: // Gujarati Vowel Sign AA
                case 0x0BBE: // Tamil Dependent Vowel Sign AA
                case 0x0BBF: // Tamil Dependent Vowel Sign I
                case 0x0BC2: // Tamil Vowel Sign UU
                case 0x0BC8: // Tamil Vowel Sign AI
                case 0x0BCD: // Tamil Sign Virama
                case 0x0C3E: // Telugu Vowel Sign AA
                case 0x0C4D: // Telugu Sign Virama
                case 0x0D82: // Sinhala Sign Anusvaraya
                case 0x0DCF: // Sinhala Vowel Sign Aela-Pilla
                    break;

                default:
                    std::cerr << "error: a TLD can only be composed of letters and numbers and dashes; problem found in \""
                        << t.toUtf8().data() << "\" -- letter: &#x" << std::hex << static_cast<int>(c.unicode()) << std::dec << "; chr(" << c.unicode() << ")\n";
                }
            }
            //else we're good
        }

        if(it->f_category_name == "international")
        {
            it->f_category = "TLD_CATEGORY_INTERNATIONAL";
        }
        else if(it->f_category_name == "professionals")
        {
            it->f_category = "TLD_CATEGORY_PROFESSIONALS";
        }
        else if(it->f_category_name == "language")
        {
            it->f_category = "TLD_CATEGORY_LANGUAGE";
        }
        else if(it->f_category_name == "groups")
        {
            it->f_category = "TLD_CATEGORY_GROUPS";
        }
        else if(it->f_category_name == "region")
        {
            it->f_category = "TLD_CATEGORY_REGION";
        }
        else if(it->f_category_name == "technical")
        {
            it->f_category = "TLD_CATEGORY_TECHNICAL";
        }
        else if(it->f_category_name == "country")
        {
            it->f_category = "TLD_CATEGORY_COUNTRY";
        }
        else if(it->f_category_name == "entrepreneurial")
        {
            it->f_category = "TLD_CATEGORY_ENTREPRENEURIAL";
        }
        else
        {
            std::cerr << "error: unknown category \"" << it->f_category_name.toUtf8().data() << "\"\n";
            exit(1);
        }

        // if within a <forbid> tag we have a reason too
        if(it->f_reason_name == "proposed")
        {
            it->f_reason = "TLD_STATUS_PROPOSED";
        }
        else if(it->f_reason_name == "deprecated")
        {
            it->f_reason = "TLD_STATUS_DEPRECATED";
        }
        else if(it->f_reason_name == "unused")
        {
            it->f_reason = "TLD_STATUS_UNUSED";
        }
        else if(it->f_reason_name == "reserved")
        {
            it->f_reason = "TLD_STATUS_RESERVED";
        }
        else if(it->f_reason_name == "infrastructure")
        {
            it->f_reason = "TLD_STATUS_INFRASTRUCTURE";
        }
        else if(!it->f_reason_name.isEmpty())
        {
            std::cerr << "error: unknown reason \"" << it->f_reason_name.toUtf8().data() << "\"\n";
            exit(1);
        }
        else
        {
            it->f_reason = "TLD_STATUS_VALID";
        }
    }
    // At time of writing it is 21 characters
    //std::cout << "longest TLD is " << max_tld_length << "\n";
}


/// The output file
QFile out_file;

/// The output text stream that writes inside the output file
QTextStream out;

/// Setup the output file and stream for easy write of the output.
void setup_output(const QString& path)
{
    out_file.setFileName(path + "/tld_data.c");
    if(!out_file.open(QIODevice::WriteOnly)) {
        std::cerr << "error: cannot open snap_path_tld.cpp output file\n";
        exit(1);
    }
    out.setDevice(&out_file);
}


/// Output UTF-8 strings using \\xXX syntax so it works in any C compiler.
void output_utf8(const QString& str)
{
    QByteArray utf8_buffer = str.toUtf8();
    const char *utf8 = utf8_buffer.data();
    int max = strlen(utf8);
    for(int i = 0; i < max; ++i)
    {
        unsigned char u(utf8[i]);
        if(u > 0x7F)
        {
            // funny looking, but to avoid problems with the next
            // character we put this one \x## inside a standalone
            // string... remember that multiple strings one after
            // another are simply concatenated in C/C++
            out << "\"\"\\x" << hex << (u & 255) << dec << "\"\"";
        }
        else
        {
            out << static_cast<char>(u);
        }
    }
}


/// Output the list of countries, each country has its own variable.
void output_countries(const country_map_t& countries)
{
    int max(0);
    for(country_map_t::const_iterator it = countries.begin();
                            it != countries.end();
                            ++it)
    {
        if(it.value() > max)
        {
            max = it.value();
        }
    }

    // first entry is used for international, etc.
    for(int i = 1; i <= max; ++i)
    {
        out << "const char tld_country" << i << "[] = \"";
        output_utf8(countries.key(i));
        out << "\";\n";
    }
}


/// Save an offset in the info table.
void save_offset(tld_info_map_t& map, const QString& tld, int offset)
{
    int e = tld.lastIndexOf('!', -2);
    QString parent = tld.left(e + 1);
    if(!map.contains(parent))
    {
        std::cerr << "error: TLD \"" << tld.toUtf8().data()
                    << "\" does not have a corresponding TLD at the previous level (i.e. \""
                    << parent.toUtf8().data() << "\").\n";
        exit(1);
    }
    if(map[parent].f_start_offset == USHRT_MAX)
    {
        map[parent].f_start_offset = offset;
    }
    map[parent].f_end_offset = offset + 1;
}


/// Prints out all the TLDs in our tld_data.c file for very fast access.
void output_tlds(tld_info_map_t& map,
                const country_map_t& countries)
{
    // to create the table below we want one entry with an
    // empty TLD and that will appear last with the info we
    // need to search level 1
    tld_info tld;
    tld.f_category_name = "international";
    tld.f_country = "";
    tld.f_level = 0;
    tld.f_tld = "";
    tld.f_inverted = "";
    tld.f_reason_name = "TLD_STATUS_VALID";
    tld.f_exception_apply_to = "";
    tld.f_offset = 0;
    tld.f_start_offset = USHRT_MAX;
    tld.f_end_offset = USHRT_MAX;

    map[""] = tld; // top-level (i.e. level 0)

    // first we determine the longest TLD in terms of levels
    // (i.e. number of periods)
    int max_level(0);
    for(tld_info_map_t::const_iterator it = map.begin();
                            it != map.end();
                            ++it)
    {
        if(max_level < it->f_level)
        {
            max_level = it->f_level;
        }
    }

    // define the offsets used with the exceptions
    int i(0);
    for(int level = max_level; level > 0; --level)
    {
        for(tld_info_map_t::iterator it = map.begin();
                                it != map.end();
                                ++it)
        {
            if(it->f_level == level)
            {
                it->f_offset = i;
                ++i;
            }
        }
    }

    // now we output the table with the largest levels first,
    // as we do so we save the index of the start and stop
    // points of each level in the previous level (hence the
    // need for a level 0 entry)
    out << "const struct tld_description tld_descriptions[] =\n{\n";
    int base_max(0);
    i = 0;
    for(int level = max_level; level > 0; --level)
    {
        for(tld_info_map_t::const_iterator it = map.begin();
                                it != map.end();
                                ++it)
        {
            if(it->f_level == level)
            {
                if(i != 0)
                {
                    out << ",\n";
                }
                unsigned short apply_to(USHRT_MAX);
                //unsigned char exception_level(USHRT_MAX);
                QString status(it->f_reason);
                if(!it->f_exception_apply_to.isEmpty()) {
                    status = "TLD_STATUS_EXCEPTION";
                    apply_to = map[it->f_exception_apply_to].f_offset;
                }
                out << "\t/* " << i << " */ { " << it->f_category.toUtf8().data()
                                    << ", " << status.toUtf8().data()
                                    << ", " << it->f_start_offset
                                    << ", " << it->f_end_offset
                                    << ", " << apply_to
                                    << ", " << it->f_level
                                    << ", \"";
                save_offset(map, it->f_inverted, i);
                // we only have to save the current level
                int e = it->f_inverted.lastIndexOf('!', -2);
                QString base(it->f_inverted.mid(e + 1, it->f_inverted.length() - e - 2));
                if(base.length() > base_max)
                {
                    base_max = base.length();
                }
                output_utf8(base);
                if(it->f_category == "TLD_CATEGORY_COUNTRY")
                {
                    out << "\", tld_country" << countries[it->f_country];
                }
                else
                {
                    out << "\", (const char *) 0";
                }
                out    << " }";
                ++i;
            }
        }
    }
    out << "\n};\n";

    out << "unsigned short tld_start_offset = " << map[""].f_start_offset << ";\n";
    out << "unsigned short tld_end_offset = " << map[""].f_end_offset << ";\n";
    out << "int tld_max_level = " << max_level << ";\n";
}


/// At this point we're not using this table.
void output_offsets(const tld_info_map_t& map,
                    const tld_info_letters_t& letters)
{
    // we know that the table always starts at zero so we skip the first
    // entry (plus the first entry is for the '%' which is not contiguous
    // with 'a')
    out << "const int tld_offsets[] = {\n";
    for(tld_info_letters_t::const_iterator it = letters.begin() + 1;
                            it != letters.end();
                            ++it)
    {
        out << "\t/* '" << static_cast<char>(it.key()) << "' */ " << it.value() << ",\n";
    }
    out << "\t/* total size */ " << map.size() << "\n};\n";
}


/// Output the tld_data.c header.
void output_header()
{
    out << "/* *** AUTO-GENERATED *** DO NOT EDIT ***\n";
    out << " * This list of TLDs was auto-generated using snap_path_parser.cpp.\n";
    out << " * Fix the parser or XML file used as input instead of this file.\n";
    out << " *\n";
    out << " * Copyright (C) 2011-2013  Made to Order Software Corp.\n";
    out << " *\n";
    out << " * This program is free software; you can redistribute it and/or modify\n";
    out << " * it under the terms of the GNU General Public License as published by\n";
    out << " * the Free Software Foundation; either version 2 of the License, or\n";
    out << " * (at your option) any later version.\n";
    out << " *\n";
    out << " * This program is distributed in the hope that it will be useful,\n";
    out << " * but WITHOUT ANY WARRANTY; without even the implied warranty of\n";
    out << " * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n";
    out << " * GNU General Public License for more details.\n";
    out << " *\n";
    out << " * You should have received a copy of the GNU General Public License\n";
    out << " * along with this program; if not, write to the Free Software\n";
    out << " * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA\n";
    out << " */\n";
    out << "#include \"tld_data.h\"\n";
    out << "#include \"libtld/tld.h\"\n";
}

/// Output the tld_data.c footer
void output_footer()
{
}


/// This function is useful to see what the heck we're working on
void output_map(const tld_info_map_t& map)
{
    for(tld_info_map_t::const_iterator it = map.begin();
                            it != map.end();
                            ++it)
    {
        std::cout << it->f_tld.toUtf8().data() << ":"
            << it->f_category_name.toUtf8().data();
        if(!it->f_country.isNull()) {
            std::cout << " (" << it->f_country.toUtf8().data() << ")";
        }
        if(!it->f_reason_name.isNull()) {
            std::cout << " [" << it->f_reason_name.toUtf8().data() << "]";
        }
        std::cout << "\n";
    }
}


} // namespace snap



/// Console tool to generate the tld_data.c file.
int main(int argc, char *argv[])
{
    if(argc != 2)
    {
        std::cerr << "error: usage 'tld_parser <path>'" << std::endl;
        exit(1);
    }
    if(strcmp(argv[1], "--help") == 0
    || strcmp(argv[1], "-h") == 0)
    {
        std::cerr << "usage: tld_parser [-<opt>] <path>" << std::endl;
        std::cerr << "where <path> is the source path where tld_data.xml is defined and where tld_data.c is saved." << std::endl;
        std::cerr << "where -<opt> can be:" << std::endl;
        std::cerr << "  --help | -h    prints out this help screen" << std::endl;
        exit(1);
    }
    snap::tld_info_map_t map;
    snap::country_map_t countries;
    //snap::tld_info_letters_t letters;
    snap::read_tlds(argv[1], map, countries);
    snap::verify_data(map);
    snap::setup_output(argv[1]);
    snap::output_header();
    snap::output_countries(countries);
    snap::output_tlds(map, countries);
    //snap::output_offsets(map, letters); -- letters is not computed
    snap::output_footer();
    //snap::output_map(map);
}


// vim: ts=4 sw=4 et
