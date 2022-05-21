/*
 * Text:
 *      src/QSerialization.cpp
 *
 * Description:
 *      Handling the serialization of structures (composites) in Qt in a way
 *      that is forward and backward compatible.
 *
 * Documentation:
 *      See each function below.
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
#include "QtSerialization/QSerialization.h"
#include "QtSerialization/QSerializationExceptions.h"
#include <QDebug>

/** \brief All the QtSerialization definitions are found in this namespace.
 *
 * It is suggested that you do not use the using command with the
 * QtSerialization namespace. Some of the classes have quite generic
 * names and you may get a clash.
 *
 * The QSerializationObject was not named QObject to avoid problems with
 * the Qt base class definition. However, other classes have fairly
 * generic names too.
 */
namespace QtSerialization
{

/** \mainpage
 *
 * \image html logo.png
 *
 * \section summary Summary
 *
 * -- \ref libqtserialization
 *
 * \par
 * \ref cpplib
 *
 * \par
 * \ref organization
 *
 * \par
 * \ref communication
 *
 * \par
 * \ref faq
 *
 * \par
 * \ref changes
 *
 * -- \ref copyright
 *
 * \section libqtserialization libQtSerialization
 *
 * \subsection cpplib Forward and Backward Serialization
 *
 * The Qt library offers a QDataStream object for serialization. It works for
 * writing and reading hard coded, non-changing data. In our world, however,
 * it is rare that data stays static between versions. Actually, it is quite
 * the opposite. It is totally unlikely that between two versions of a software
 * that the serialized data be compatible between both versions.
 *
 * This library was created to have a relatively easy way (compared to
 * QDataStream, it should be as easy) to serialize data to a stream and
 * re-read it from that stream. And when versions of the software change,
 * the data can still be read and written and have your software work
 * as expected (i.e. not crash, just ignore data that it doesn't
 * understand.)
 *
 * If you add new fields, then you should properly initialize them and not
 * expect them to exist in the serialized data.
 *
 * If you remove fields, then they are simply ignored on load and of course
 * they cannot be saved since they don't exist.
 *
 * If you rename a field, then you may have to do a little bit of work to
 * handle the old name as if it were the new name (i.e. in general this means
 * add one entry to the loader.)
 *
 * \todo
 * Version 0.1.0 of the library does not yet fully support forward and
 * backward compatibility.
 *
 * \subsection organization The libQtSerialization organization 
 *
 * \li Serializing
 *
 * \par
 * The library offers the QtSerialization::QWriter class to create serialized
 * data from your classes. The QWriter is self contained so no other classes
 * are required to create serialized data.
 *
 * \par
 * There are a set writeTag() functions one can use to write the basic type
 * of data to the output stream. New writeTag() functions can be defined
 * to support additional types (i.e. QPoint, QMatrix, ...)
 *
 * \li Unserializing
 *
 * \par
 * The library offers the QtSerialization::QReader class to read serialized
 * data from a stream and save the data in your class.
 *
 * \par
 * Contrary to the QWriter class, the reader is not self contained. It requires
 * defining a set QField objects in QComposite objects. At this point, the
 * supported fields are QFieldBasicType, QFieldString, and user defined
 * QFieldTag.
 *
 * \par
 * The user defined tags are particularly useful when you have a class with
 * either pre-defined sub-classes or arrays of sub-classes.
 *
 * \li Reason for having a separate QReader and QWriter
 *
 * \par
 * It is quite often that your software will need to either read or write
 * serialized data, but not both. Having the reader and writer separate
 * allows you to save on time and memory by creating only one or the other.
 *
 * \par
 * Another important aspect of this implementation: it attempts to limit
 * the number of memory allocations in the process of saving or loading
 * the data.
 *
 * \li Drawback of the separate Reader/Writer
 *
 * \par
 * There is one drawback in having the reader and the writer separated as it
 * is. You may save a field named "Test" and trying to reload it as "Tset".
 * It will fail, of course. The library does not generate an error for a
 * missing field since it is backward/forward compatible that way...
 *
 * \par
 * There isn't a good way to define the name of the fields in one single
 * place in the serialization library. However, you can avoid this problem
 * by using variables for all the names instead of entering the name each
 * time.
 *
 * \code
 * // this could be a global or a static const in your class
 * namespace {
 *   const char *test_field = "Test";
 * }
 *
 * ...
 * QtSerialization::writeTag(writer, test_field, f_test_value);
 * ...
 * QtSerialization::QFieldInt32 f1(composite, test_field, f_test_value);
 * \endcode
 *
 * \li Exceptions
 *
 * \par
 * The library defines a set of exceptions that are raised whenever an error
 * occurs. The library is actually expected to never raise an exception, however
 * if you tweak the serialized data or read/write between versions and somehow
 * the backword/forward compatibility is not correctly handled, then an
 * exception is raised.
 *
 * \par
 * It is possible to catch all the library exceptions using the base exception:
 * QException.
 *
 * \subsection communication Communication between objects while reading
 *
 * The writing is very straight forward. You call write() with each field that
 * you want to save. For arrays, you simple loop through your data and call
 * write on each item in the array. For sub-objects (pointer to another
 * class from an object) you create a write function on that class and
 * call it from the parent.
 *
 * This is great, but the read does not benefit from such simplicity. It would
 * be simpler if we did not have to support for dynamic array (i.e. the tree
 * of objects and sub-objects was known, always fixed.) The QReader was written
 * to support any number of cases with any number of parent/child relationships.
 *
 * The reader makes use of a QComposite object. This object is an array of
 * named fields. The names that are saved in the n attribute of the \<v> tags
 * in the serialized data. For example, an object may have a field named
 * house which would look like this:
 *
 * \code
 * ...<v n="house">Large house on the corner</v>...
 * \endcode
 *
 * In this case, the QComposite object would include a QFieldString named
 * "house". When the QReader hits the tag and reads the n attribute, it
 * then asks the QComposite object to send a signal to the corresponding
 * QFieldString which in turn saves the data "Large house on the corner"
 * in the field as defined in the QFieldString object.
 *
 * \msc
 * QReader,QComposite,QField;
 * QReader=>QReader [label="readTag()"];
 * QReader=>QComposite [label="readField()"];
 * QComposite=>QField [label="read()"];
 * QField=>QReader [label="readText()"];
 * QReader>>QField [label="return"];
 * QField=>QField [label="convertData()"];
 * QField>>QComposite [label="return"];
 * QComposite>>QReader [label="return"];
 * \endmsc
 *
 * The read() function of the QField class is a pure virtual. A specific field
 * will implement it and parse the data appropriately. The data may be a tag
 * and text, or just text. In case of the basic types, the only data expected
 * in a field is text. The event sample shown here presents the QField reading
 * text between the open &amp; close tag, converting and saving that data
 * and then returning to the reader.
 *
 * The following shows you the process when you make your object capable of
 * reading some fields in a specialized way. As you can see, when the QField
 * read function is called, it then forwards that signal to your object
 * readTag() function. There you may read data, tags or text, from the
 * QReader.
 *
 * \msc
 * QReader,QComposite,QField,QSerializationObject;
 * QReader=>QReader [label="readTag()"];
 * QReader=>QComposite [label="readField()"];
 * QComposite=>QField [label="read()"];
 * QField=>QSerializationObject [label="readTag()"];
 * QSerializationObject=>QReader [label="readTag() or readText()"];
 * QReader>>QSerializationObject [label="return"];
 * QSerializationObject>>QField [label="return"];
 * QField>>QComposite [label="return"];
 * QComposite>>QReader [label="return"];
 * \endmsc
 *
 * \note
 * If you implement a readTag() function that reads tags, remember that
 * you still must make sure that when you return, you read the \</v> tag
 * closing your tag as expected by the QReader. Otherwise you will be out
 * of sync. and the process will not continue to work.
 *
 * \subsection faq FAQ
 *
 * \li Do you have an example of usage?
 *
 * \par
 * Yes. The tests folder has a serialize.cpp file which in effect is an
 * example of usage. It shows all the different cases that one would
 * want to use.
 *
 * \par
 * The Snap! C++ snap_uri.cpp file will also include an example. This is not
 * yet available though.
 *
 * \li Why is my data saved but not reloaded?
 *
 * \par
 * You probably want to check the name and make sure it is the same that you
 * use with the QWriter and the QReader. If the name doesn't match then the
 * load will miss the saved data.
 *
 * \li Why is my number not loading properly?
 *
 * \par
 * The library takes sign in account. If you save a qint8 and then reload it
 * as a quint8, then any negative number is likely to have a hard time.
 * Similarly, if you were saving a qint32 before and now are reloading that
 * value as a qint8, you may be losing many bits.
 *
 * \par
 * Floating points are saved as such so the value will not be exact unless
 * the value represents an integer (i.e. 3.0) or a value that can be
 * represented exactly in decimal. In a future version we will support
 * saving floating points as decimal numbers instead. That way you will
 * not lose any bit.
 *
 * \section changes Changes between versions
 *
 * \li Version 0.2.0
 *
 * . Added the bool type to the supported basic types.
 * . Fixed the unsigned integers conversion (use the U!)
 *
 * \li Version 0.1.0
 *
 * . First working version, although it does not fully support forward
 *   and backward compatibility.
 *
 * \section copyright libQtSerialization copyright and license
 *
 * Copyright (c) 2012-2022  Made to Order Software Corp.  All Rights Reserved
 *
 * http://snapwebsites.org/<br/>
 * contact@m2osw.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */










/** \brief Retrieve the major version number.
 *
 * This function returns the major version number of the library at the
 * time it was compiled.
 *
 * \return The major version number.
 */
int QLibraryVersionMajor()
{
    return QT_SERIALIZATION_LIBRARY_VERSION_MAJOR;
}

/** \brief Retrieve the minor version number.
 *
 * This function returns the minor version number of the library at the
 * time it was compiled.
 *
 * \return The minor version number.
 */
int QLibraryVersionMinor()
{
    return QT_SERIALIZATION_LIBRARY_VERSION_MINOR;
}

/** \brief Retrieve the patch version number.
 *
 * This function returns the patch version number of the library at the
 * time it was compiled.
 *
 * \return The patch version number.
 */
int QLibraryVersionPatch()
{
    return QT_SERIALIZATION_LIBRARY_VERSION_PATCH;
}

/** \brief Retrieve the library version.
 *
 * This function returns the full version (major.minor.patch) of the
 * library at the time it was compiled.
 *
 * \return The full library version as a string.
 */
const char *QLibraryVersion()
{
    return QT_SERIALIZATION_LIBRARY_VERSION_STRING;
}


/** \brief Retrieve the version of files created by this library.
 *
 * This function returns the file format version. In other words, files
 * generated by this library will be stamped with this version number.
 * When the format changes, this version increases, however, older files
 * should always be loadable by the newer versions and vice versa.
 *
 * \return The file format version of this library.
 */
int QLibraryFileFormatVersion()
{
    return FILE_FORMAT_VERSION;
}


} // namespace QtSerialization
// vim: ts=4 sw=4 et
