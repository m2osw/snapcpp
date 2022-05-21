/*
 * Header:
 *      include/QtSerialization/QSerializationExceptions.h
 *
 * Description:
 *      Definition of the serialization library exceptions.
 *
 * Documentation:
 *      See the corresponding .cpp file.
 *
 * License:
 *      Copyright (c) 2012-2022  Made to Order Software Corp.  All Rights Reserved
 * 
 *      https://snapwebsites.org/
 *      contact@m2osw.com
 * 
 *      Permission is hereby granted, free of charge, to any person obtaining a
 *      copy of this software and associated documentation files (the
 *      "Software"), to deal in the Software without restriction, including
 *      without limitation the rights to use, copy, modify, merge, publish,
 *      distribute, sublicense, and/or sell copies of the Software, and to
 *      permit persons to whom the Software is furnished to do so, subject to
 *      the following conditions:
 *
 *      The above copyright notice and this permission notice shall be included
 *      in all copies or substantial portions of the Software.
 *
 *      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 *      OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *      MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *      IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 *      CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 *      TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *      SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef QT_SERIALIZATION_EXCEPTIONS_H
#define QT_SERIALIZATION_EXCEPTIONS_H

#include <stdexcept>

namespace QtSerialization
{


/** \brief Base class for the serialization exceptions.
 *
 * This is the base class for all the serialization exceptions. It can be
 * used to catch all the exceptions that this library generates (although
 * some lower layer exceptions may also occur such as std::bad_alloc).
 *
 * Note that means all the exceptions derive from the standard run-time
 * error exception.
 */
class QException : public std::runtime_error
{
public:
    /** \brief The exception constructor.
     *
     * This constructor initialize the exception with a what_msg string
     * describing the exception in more details.
     *
     * The string is expected to be a static string although it
     * doesn't have to be.
     *
     * \param[in] what_msg  Details about the exception.
     */
	QException(const char *what_msg)
        : runtime_error(what_msg)
    {
    }
};

/** \brief A read or a write to a stream failed.
 *
 * When the system needs to write data to the user stream or read data
 * from the user stream, then this error occurs if the writing or reading
 * do not work as expected.
 */
class QExceptionIOError : public QException
{
public:
    /** \brief The exception constructor.
     *
     * This constructor initialize the exception with a what_msg string
     * describing the exception in more details.
     *
     * The string is expected to be a static string although it
     * doesn't have to be.
     *
     * \param[in] what_msg  Details about the exception.
     */
	QExceptionIOError(const char *what_msg)
        : QException(what_msg)
    {
    }
};

/** \brief A reference or pointer is NULL when it should not be.
 *
 * Several classes expect pointers (or references) to objects that they
 * are to use while working on the serialization process. If the pointer
 * of one such object is discovered to be a NULL pointer, this exception
 * is raised.
 *
 * Note that all pointers are not checked so you may also get a SEGV error
 * in case a pointer is NULL and dereferenced.
 */
class QExceptionNullReference : public QException
{
public:
    /** \brief The exception constructor.
     *
     * This constructor initialize the exception with a what_msg string
     * describing the exception in more details.
     *
     * The string is expected to be a static string although it
     * doesn't have to be.
     *
     * \param[in] what_msg  Details about the exception.
     */
	QExceptionNullReference(const char *what_msg)
        : QException(what_msg)
    {
    }
};

/** \brief A value is not yet defined.
 *
 * Right after initialization, a certain number of values may not yet be
 * defined (because there are no good defaults for them.) Trying to retrieve
 * these values generate this exception.
 *
 * \sa QFieldTag
 */
class QExceptionNotDefined : public QException
{
public:
    /** \brief The exception constructor.
     *
     * This constructor initialize the exception with a what_msg string
     * describing the exception in more details.
     *
     * The string is expected to be a static string although it
     * doesn't have to be.
     *
     * \param[in] what_msg  Details about the exception.
     */
	QExceptionNotDefined(const char *what_msg)
        : QException(what_msg)
    {
    }
};

/** \brief Each field in a given composite must have a unique name.
 *
 * It is not possible to define more than one field with the same
 * name. This is important for the read process since the system
 * has no way to choose which field to load if two fields have the
 * same name.
 *
 * This being said, if you need such a feature, using the tag option
 * you can then handle different cases with multiple fields named
 * the same.
 *
 * \sa QFieldTag
 */
class QExceptionAlreadyDefined : public QException
{
public:
    /** \brief The exception constructor.
     *
     * This constructor initialize the exception with a what_msg string
     * describing the exception in more details.
     *
     * The string is expected to be a static string although it
     * doesn't have to be.
     *
     * \param[in] what_msg  Details about the exception.
     */
	QExceptionAlreadyDefined(const char *what_msg)
        : QException(what_msg)
    {
    }
};

/** \brief The data read from the stream is invalid.
 *
 * This exception is raised whenever the data read from the input
 * does not correspond to a valid serialization process. This
 * means an invalid tag, an invalid attribute, an invalid quote,
 * an invalid character in an attribute or text data, etc.
 *
 * In normal processing this exception should never be raised.
 */
class QExceptionInvalidRead : public QException
{
public:
    /** \brief The exception constructor.
     *
     * This constructor initialize the exception with a what_msg string
     * describing the exception in more details.
     *
     * The string is expected to be a static string although it
     * doesn't have to be.
     *
     * \param[in] what_msg  Details about the exception.
     */
	QExceptionInvalidRead(const char *what_msg)
        : QException(what_msg)
    {
    }
};

/** \brief Unget buffer of the QReader is full.
 *
 * This is an internal exception that should never happen. It
 * is raised if the f_unget character is not used before another
 * character is unget().
 *
 * It should never happen because we always read a new character
 * before we attempt to unget a character.
 */
class QExceptionBufferIsFull : public QException
{
public:
    /** \brief The exception constructor.
     *
     * This constructor initialize the exception with a what_msg string
     * describing the exception in more details.
     *
     * The string is expected to be a static string although it
     * doesn't have to be.
     *
     * \param[in] what_msg  Details about the exception.
     */
	QExceptionBufferIsFull(const char *what_msg)
        : QException(what_msg)
    {
    }
};

/** \brief Part of the library is not implemented yet.
 *
 * Some parts of the library are not yet implemented and thus
 * this exception is raised. It should get implemented as we
 * move forward. This exception is mainly for backward
 * compatibility (i.e. older versions may generate this
 * exception when using a newer feature that required code
 * that did not yet exist in the older version.)
 */
class QExceptionNotSupported : public QException
{
public:
    /** \brief The exception constructor.
     *
     * This constructor initialize the exception with a what_msg string
     * describing the exception in more details.
     *
     * The string is expected to be a static string although it
     * doesn't have to be.
     *
     * \param[in] what_msg  Details about the exception.
     */
	QExceptionNotSupported(const char *what_msg)
        : QException(what_msg)
    {
    }
};

/** \brief A tag was found but it was not expected there.
 *
 * The current version supports two tags: \<r> and \<v>.
 * When reading the root tag, it must be \<r> or this
 * exception is raised.
 *
 * With time, other combinaisons may raise this exception.
 */
class QExceptionInvalidTag : public QException
{
public:
    /** \brief The exception constructor.
     *
     * This constructor initialize the exception with a what_msg string
     * describing the exception in more details.
     *
     * The string is expected to be a static string although it
     * doesn't have to be.
     *
     * \param[in] what_msg  Details about the exception.
     */
	QExceptionInvalidTag(const char *what_msg)
        : QException(what_msg)
    {
    }
};




} // namespace QtSerialization
#endif
// #ifndef QT_SERIALIZATION_EXCEPTIONS_H
// vim: ts=4 sw=4 et
