/* message.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

/*

Copyright (c) 2005-2017 Made to Order Software Corp.

http://snapwebsites.org/project/as2js

Permission is hereby granted, free of charge, to any
person obtaining a copy of this software and
associated documentation files (the "Software"), to
deal in the Software without restriction, including
without limitation the rights to use, copy, modify,
merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the
following conditions:

The above copyright notice and this permission notice
shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include "as2js/message.h"


namespace as2js
{

namespace
{
    MessageCallback *   g_message_callback = nullptr;
    message_level_t     g_maximum_message_level = message_level_t::MESSAGE_LEVEL_INFO;
    int                 g_warning_count = 0;
    int                 g_error_count = 0;
}
// no name namespace


/** \brief Create a message object with the specified information.
 *
 * This function generates a message object that can be used to generate
 * a message with the << operator and then gets sent to the client using
 * the message callback function on destruction.
 *
 * The level can be set to any one of the message levels available in
 * the message_level_t enumeration. The special MESSAGE_LEVEL_OFF value
 * can be used to avoid the message altogether (can be handy when you
 * support a varying message level.)
 *
 * \param[in] message_level  The level of the message.
 * \param[in] error_code  An error code to print in the output message.
 * \param[in] pos  The position to which the message applies.
 */
Message::Message(message_level_t message_level, err_code_t error_code, Position const& pos)
    //: stringstream() -- auto-init
    : f_message_level(message_level)
    , f_error_code(error_code)
    , f_position(pos)
{
}


/** \brief Create a message object with the specified information.
 *
 * This function is an overload of the default constructor that does not
 * include the position information. This is used whenever we generate
 * an error from outside of the node tree, parser, etc.
 *
 * \param[in] message_level  The level of the message.
 * \param[in] error_code  An error code to print in the output message.
 */
Message::Message(message_level_t message_level, err_code_t error_code)
    //: stringstream() -- auto-init
    : f_message_level(message_level)
    , f_error_code(error_code)
    //, f_position()
{
}


/** \brief Output the message created with the << operators.
 *
 * The destructor of the message object is where things happen. This function
 * prints out the message that was built using the different << operators
 * and the parameters specified in the constructor.
 *
 * The result is then passed to the message callback. If you did not setup
 * that function, the message is lost.
 *
 * If the level of the message was set to MESSAGE_LEVEL_OFF (usualy via
 * a command line option) then the message callback does not get called.
 */
Message::~Message()
{
    // actually emit the message
    if(g_message_callback                           // there is a callback?
    && message_level_t::MESSAGE_LEVEL_OFF != f_message_level         // level is off?!
    && f_message_level <= g_maximum_message_level   // level is large enough?
    && rdbuf()->in_avail() != 0)                    // there is a message?
    {
        if(f_position.get_filename().empty())
        {
            f_position.set_filename("unknown-file");
        }
        if(f_position.get_function().empty())
        {
            f_position.set_function("unknown-func");
        }

        switch(f_message_level)
        {
        case message_level_t::MESSAGE_LEVEL_FATAL:
        case message_level_t::MESSAGE_LEVEL_ERROR:
            ++g_error_count;
            break;

        case message_level_t::MESSAGE_LEVEL_WARNING:
            ++g_warning_count;
            break;

        // others are not currently counted
        default:
            break;

        }

        g_message_callback->output(f_message_level, f_error_code, f_position, str());
    }
}


/** \brief Append an char string.
 *
 * This function appends an char string to the message.
 *
 * \param[in] s  A character string.
 *
 * \return A reference to the message.
 */
Message& Message::operator << (char const *s)
{
    // we assume UTF-8 because in our Snap environment most everything is
    static_cast<std::stringstream&>(*this) << s;
    return *this;
}


/** \brief Append an wchar_t string.
 *
 * This function appends an wchar_t string to the message.
 *
 * \param[in] s  A wchar_t string.
 *
 * \return A reference to the message.
 */
Message& Message::operator << (wchar_t const *s)
{
    String string;
    string.from_wchar(s);
    static_cast<std::stringstream&>(*this) << string.to_utf8();
    return *this;
}


/** \brief Append an std::string value.
 *
 * This function appends an std::string value to the message.
 *
 * \param[in] s  An std::string value.
 *
 * \return A reference to the message.
 */
Message& Message::operator << (std::string const& s)
{
    static_cast<std::stringstream&>(*this) << s;
    return *this;
}


/** \brief Append an std::wstring value.
 *
 * This function appends an std::wstring value to the message.
 *
 * \param[in] s  An std::wstring value.
 *
 * \return A reference to the message.
 */
Message& Message::operator << (std::wstring const& s)
{
    String string;
    string.from_wchar(s.c_str(), s.length());
    static_cast<std::stringstream&>(*this) << string.to_utf8();
    return *this;
}


/** \brief Append a String value.
 *
 * This function appends a String value to the message.
 *
 * \param[in] s  A String value.
 *
 * \return A reference to the message.
 */
Message& Message::operator << (String const& s)
{
    static_cast<std::stringstream&>(*this) << s.to_utf8();
    return *this;
}


/** \brief Append a char value.
 *
 * This function appends a character to the message.
 *
 * \param[in] v  A char value.
 *
 * \return A reference to the message.
 */
Message& Message::operator << (char const v)
{
    static_cast<std::stringstream&>(*this) << v;
    return *this;
}


/** \brief Append a signed char value.
 *
 * This function appends a signed char value to the message.
 *
 * \param[in] v  A signed char value.
 *
 * \return A reference to the message.
 */
Message& Message::operator << (signed char const v)
{
    static_cast<std::stringstream&>(*this) << static_cast<int>(v);
    return *this;
}


/** \brief Append a unsigned char value.
 *
 * This function appends a unsigned char value to the message.
 *
 * \param[in] v  A unsigned char value.
 *
 * \return A reference to the message.
 */
Message& Message::operator << (unsigned char const v)
{
    static_cast<std::stringstream&>(*this) << static_cast<int>(v);
    return *this;
}


/** \brief Append a signed short value.
 *
 * This function appends a signed short value to the message.
 *
 * \param[in] v  A signed short value.
 *
 * \return A reference to the message.
 */
Message& Message::operator << (signed short const v)
{
    static_cast<std::stringstream&>(*this) << static_cast<int>(v);
    return *this;
}


/** \brief Append a unsigned short value.
 *
 * This function appends a unsigned short value to the message.
 *
 * \param[in] v  A unsigned short value.
 *
 * \return A reference to the message.
 */
Message& Message::operator << (unsigned short const v)
{
    static_cast<std::stringstream&>(*this) << static_cast<int>(v);
    return *this;
}


/** \brief Append a signed int value.
 *
 * This function appends a signed int value to the message.
 *
 * \param[in] v  A signed int value.
 *
 * \return A reference to the message.
 */
Message& Message::operator << (signed int const v)
{
    static_cast<std::stringstream&>(*this) << v;
    return *this;
}


/** \brief Append a unsigned int value.
 *
 * This function appends a unsigned int value to the message.
 *
 * \param[in] v  A unsigned int value.
 *
 * \return A reference to the message.
 */
Message& Message::operator << (unsigned int const v)
{
    static_cast<std::stringstream&>(*this) << v;
    return *this;
}


/** \brief Append a signed long value.
 *
 * This function appends a signed long value to the message.
 *
 * \param[in] v  A signed long value.
 *
 * \return A reference to the message.
 */
Message& Message::operator << (signed long const v)
{
    static_cast<std::stringstream&>(*this) << v;
    return *this;
}


/** \brief Append a unsigned long value.
 *
 * This function appends a unsigned long value to the message.
 *
 * \param[in] v  A unsigned long value.
 *
 * \return A reference to the message.
 */
Message& Message::operator << (unsigned long const v)
{
    static_cast<std::stringstream&>(*this) << v;
    return *this;
}


/** \brief Append a signed long long value.
 *
 * This function appends a signed long long value to the message.
 *
 * \param[in] v  A signed long long value.
 *
 * \return A reference to the message.
 */
Message& Message::operator << (signed long long const v)
{
    static_cast<std::stringstream&>(*this) << v;
    return *this;
}


/** \brief Append an Int64 value.
 *
 * This function appends the value saved in an Int64 value.
 *
 * \param[in] v  An as2js::Int64 value.
 */
Message& Message::operator << (Int64 const v)
{
    static_cast<std::stringstream&>(*this) << v.get();
    return *this;
}


/** \brief Append a unsigned long long value.
 *
 * This function appends a unsigned long long value to the message.
 *
 * \param[in] v  A unsigned long long value.
 *
 * \return A reference to the message.
 */
Message& Message::operator << (unsigned long long const v)
{
    static_cast<std::stringstream&>(*this) << v;
    return *this;
}


/** \brief Append a float value.
 *
 * This function appends a float value to the message.
 *
 * \param[in] v  A float value.
 *
 * \return A reference to the message.
 */
Message& Message::operator << (float const v)
{
    static_cast<std::stringstream&>(*this) << v;
    return *this;
}


/** \brief Append a double value.
 *
 * This function appends a double value to the message.
 *
 * \param[in] v  A double value.
 *
 * \return A reference to the message.
 */
Message& Message::operator << (double const v)
{
    static_cast<std::stringstream&>(*this) << v;
    return *this;
}


/** \brief Append a Float64 value.
 *
 * This function appends the value saved in an Float64 value.
 *
 * \param[in] v  An as2js::Float64 value.
 */
Message& Message::operator << (Float64 const v)
{
    static_cast<std::stringstream&>(*this) << v.get();
    return *this;
}


/** \brief Append a Boolean value.
 *
 * This function appends a Boolean value to the message as a 0 or a 1.
 *
 * \param[in] v  A Boolean value.
 *
 * \return A reference to the message.
 */
Message& Message::operator << (bool const v)
{
    static_cast<std::stringstream&>(*this) << static_cast<int>(v);;
    return *this;
}


/** \brief Setup the callback so tools can receive error messages.
 *
 * This function is used by external processes to setup a callback. The
 * callback receives the message output as generated by the Message
 * class.
 *
 * \sa configure()
 */
void Message::set_message_callback(MessageCallback *callback)
{
    g_message_callback = callback;
}


/** \brief Define the maximum level a message can be to be displayed.
 *
 * This function is used to change the maximum level a message can
 * be in order to be displayed. Messages with a larger level are
 * completely ignored.
 *
 * Note that errors and fatal errors cannot be ignored using this
 * mechanism (i.e. the smallest possible value for max_level is
 * MESSAGE_LEVEL_ERROR.)
 *
 * \param[in] max_level  The maximum level a message can have.
 */
void Message::set_message_level(message_level_t max_level)
{
    g_maximum_message_level = max_level < message_level_t::MESSAGE_LEVEL_ERROR
                            ? message_level_t::MESSAGE_LEVEL_ERROR
                            : max_level;
}


/** \brief The number of warnings that were found so far.
 *
 * This function returns the number of warnings that were
 * processed so far.
 *
 * Note that this number is a global counter and it cannot be reset.
 *
 * \return The number of warnings that were processed so far.
 */
int Message::warning_count()
{
    return g_warning_count;
}


/** \brief The number of errors that were found so far.
 *
 * This function returns the number of errors and fatal errors that were
 * processed so far.
 *
 * Note that this number is a global counter and it cannot be reset.
 *
 * \return The number of errors that were processed so far.
 */
int Message::error_count()
{
    return g_error_count;
}




}
// namespace as2js

// vim: ts=4 sw=4 et
