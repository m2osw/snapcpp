/* message.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2014 */

/*

Copyright (c) 2005-2014 Made to Order Software Corp.

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
 * \param[in] file  The name of the source file that message was generated from.
 * \param[in] func  The name of the function that message was generated from.
 * \param[in] line  The line number that message was generated from.
 */
Message::Message(message_level_t message_level, char const *file, char const *func, int line)
    : f_message_level(message_level)
    , f_file(file)
    , f_func(func)
    , f_line(line)
    //, f_message() -- auto-init
{
}


///* \brief Copy a message in another.
// *
// * In some cases copy messages get from one Message object to another.
// *
// * \param[in] rhs  The existing message to copy.
// */
//Message::Message(Message const& rhs)
//    : f_message_level(rhs.f_message_level)
//    , f_file(rhs.f_file)
//    , f_func(rhs.f_func)
//    , f_line(rhs.f_line)
//    //, f_message() -- auto-init
//{
//    f_message << rhs.f_message.str();
//}


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
    if(g_message_callback && f_message_level != MESSAGE_LEVEL_OFF && f_message.rdbuf()->in_avail() != 0)
    {
        if(f_file == nullptr)
        {
            f_file = "unknown-file";
        }
        if(f_func == nullptr)
        {
            f_func = "unknown-func";
        }

        switch(f_message_level)
        {
        case MESSAGE_LEVEL_FATAL:
        case MESSAGE_LEVEL_ERROR:
            ++g_error_count;
            break;

        case MESSAGE_LEVEL_WARNING:
            ++g_warning_count;
            break;

        // others are not currently counted
        default:
            break;

        }

        g_message_callback->output(f_message_level, f_file, f_func, f_line, f_message.str());
    }
}


/** \brief Append an char string.
 *
 * This function appends an char string to the message.
 *
 * \param[in] v  An char string.
 *
 * \return A reference to the message.
 */
Message& Message::operator << (char const *s)
{
    // we assume UTF-8 because in our Snap environment most everything is
    f_message << s;
    return *this;
}


/** \brief Append an wchar_t string.
 *
 * This function appends an wchar_t string to the message.
 *
 * \param[in] v  An wchar_t string.
 *
 * \return A reference to the message.
 */
Message& Message::operator << (wchar_t const *s)
{
    String str;
    str.from_wchar(s);
    f_message << str.to_utf8();
    return *this;
}


/** \brief Append an std::string value.
 *
 * This function appends an std::string value to the message.
 *
 * \param[in] v  An std::string value.
 *
 * \return A reference to the message.
 */
Message& Message::operator << (std::string const& s)
{
    f_message << s.c_str();
    return *this;
}


/** \brief Append an std::wstring value.
 *
 * This function appends an std::wstring value to the message.
 *
 * \param[in] v  An std::wstring value.
 *
 * \return A reference to the message.
 */
Message& Message::operator << (std::wstring const& s)
{
    String str;
    str.from_wchar(s.c_str(), s.length());
    f_message << str.to_utf8();
    return *this;
}


/** \brief Append a String value.
 *
 * This function appends a String value to the message.
 *
 * \param[in] v  A String value.
 *
 * \return A reference to the message.
 */
Message& Message::operator << (String const& s)
{
    f_message << s.to_utf8();
    return *this;
}


/** \brief Append a char value.
 *
 * This function appends a char value to the message.
 *
 * \param[in] v  A char value.
 *
 * \return A reference to the message.
 */
Message& Message::operator << (char const v)
{
    f_message << static_cast<int>(v);
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
    f_message << static_cast<int>(v);
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
    f_message << static_cast<int>(v);
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
    f_message << static_cast<int>(v);
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
    f_message << static_cast<int>(v);
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
    f_message << v;
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
    f_message << v;
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
    f_message << v;
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
    f_message << v;
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
    f_message << v;
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
    f_message << v;
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
    f_message << v;
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
    f_message << v;
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
    f_message << static_cast<int>(v);
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


/** \brief The number of warnings that were found so far.
 *
 * This function returns the number of warnings that were
 * processed so far.
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
 * \return The number of errors that were processed so far.
 */
int Message::error_count()
{
    return g_error_count;
}




}
// namespace as2js

// vim: ts=4 sw=4 et
