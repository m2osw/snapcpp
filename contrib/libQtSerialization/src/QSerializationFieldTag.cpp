/*
 * Text:
 *      src/QSerializationFieldTag.cpp
 *
 * Description:
 *      Handling of the user specialized field class. This class allows
 *      programmers to create sub-fields (saving a tree of objects) and
 *      arrays (saving a set of children.)
 *
 * Documentation:
 *      See each function below.
 *
 * License:
 *      Copyright (c) 2012-2017 Made to Order Software Corp.
 *
 *      http://snapwebsites.org/
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

#include "QtSerialization/QSerializationFieldTag.h"
#include "QtSerialization/QSerializationExceptions.h"

namespace QtSerialization
{


/** \class QFieldTag
 * \brief Handling of user tags.
 *
 * This class allows your code to provide a function pointer (via a
 * derivation and a virtual function) to a composite. That function
 * is called whenever a field with that name is found in the input
 * data. The function can then take over the reading of the data
 * as required (i.e. readText() and readTag() can be called.)
 *
 * The user class must be derived from QSerializationObject which
 * is defined in the QSerializationFieldTag.h header file. This
 * class is an interface with one function: readTag(). It is expected
 * to be declared in your class and is called whenever a field with
 * that name is read from the input.
 *
 * The function can be used with fields of different names. The
 * function has a parameter named \p name which represents the
 * name of the field being processed. At a later time we may look
 * into a way to offer to call any function on the object creating
 * the composite. That way we could avoid the extra tests on the
 * name parameter. (BOOST offers such a capability so we could
 * offer it too.)
 */


/** \brief Initialize a user field tag.
 *
 * This constructor accepts the composite and name of the field as
 * expected by QField and it also takes a pointer to a user object.
 *
 * The composite and name parameters are simply passed to the QField
 * constructor.
 *
 * The \p obj parameter is a pointer to one of your objects. For
 * objects that don't exist when loading said tag, then it generally
 * points to the parent which is responsible for creating the object
 * and passing the data along.
 *
 * In the serialize.cpp test we can see that the implementation of
 * the T3C1::readTag() includes testing two names: "test3.1" and
 * "test3.2". When the readTag() function is called for "test3.1"
 * it creates a set of fields, including test2_2() which connects
 * the "test3.2" field with this object. Objects named "test3.2"
 * are saved as an array in the serialized data. Each time a new
 * one is loaded, we need to create a new T3C2 object. We then
 * call the readTag() function of the T3C2 object which in turn
 * reads all the fields of the T3C2 object.
 *
 * \code
 * virtual void readTag(const QString& name, QtSerialization::QReader& r)
 * {
 *     if(name == "test3.1") {
 *         QtSerialization::QComposite comp;
 *         QtSerialization::QFieldString f2(comp, "string L1", f_string);
 *         // level 3 already exists so we can directly call its readTag() function
 *         QtSerialization::QFieldTag test2_2(comp, "test3.2", this);
 *         r.read(comp);
 *         f_pos = 0;
 *     }
 *     else if(name == "test3.2") {
 *         if(f_pos < LEVEL2_MAX) {
 *             f_level2[f_pos] = QSharedPointer<T3C2>(new T3C2(g_org[f_pos]));
 *             f_level2[f_pos]->readTag(name, r);
 *             ++f_pos;
 *         }
 *         else {
 *             printf("error: too many level2 entries?!\n");
 *         }
 *     }
 * }
 * \endcode
 *
 * \param[in] composite  The parent composite of this field.
 * \param[in] name  The name of the field as defined by n="..." in the \<v> tag.
 * \param[in] obj  A pointer to your object, usually \p this.
 */
QFieldTag::QFieldTag(QComposite& composite, const QString& name, QSerializationObject *obj)
    : QField(composite, name),
      f_name(name),
      f_obj(obj)
{
    // with g++ version 7.0+ testing a reference pointer generates a warning!
    //if(&composite == nullptr) {
    //    throw QExceptionNullReference("the composite reference cannot be a nullptr pointer");
    //}
    if(f_obj == nullptr) {
        throw QExceptionNullReference("the object pointer cannot be a nullptr pointer");
    }
}


/** \brief Implementation of the read function.
 *
 * This is the QFieldTag implementation of the read() function.
 *
 * This function simply calls your readTag() function defined as the
 * implementation of the readTag() pure virtual function of the
 * QSerializationObject class.
 *
 * The readTag() function is called with the same name as was given
 * to the object when it was initialized (see constructor.)
 *
 * \param[in] r  The QReader being used to read all the data.
 */
void QFieldTag::read(QReader& r)
{
    f_obj->readTag(f_name, r);
    // closing tag was read by callee
}


/** \class QSerializationObject
 * \brief A serialization object.
 *
 * The QSerializationObject class is defined as an interface that you
 * derive from to create fields accepting user defined tags.
 */


/** \fn QSerializationObject::~QSerializationObject();
 * \brief Clean up the serialization object.
 *
 * This function is here to ensure that the virtual table is properly
 * handled.
 */


/** \fn void QSerializationObject::readTag(const QString& name, QReader& r);
 * \brief Read one tag.
 *
 * This pure virtual function is called whenever the QReader reads one field
 * with the corresponding name. You must implement your own version of this
 * function.
 *
 * You may, for example, call the r.readText() function and then parse the
 * resulting string, returned by r.text(), to define the variable members
 * that this field represents.
 *
 * This function must return only after reading the closing tag of the field
 * being read. If you miss doing so, the reader will throw and exception.
 *
 * If you are reading a simple text field, this means calling readTag() before
 * returning. The following shows the implementation of the string field:
 *
 * \code
 * void QFieldString::read(QReader& r)
 * {
 *     r.readText();
 *     f_field = r.text();
 *     r.readTag();
 * }
 * \endcode
 *
 * As we can see, the function reads text with a call to r.readText(), then
 * it copies the text to the string field with r.text(), and finally, it
 * calls r.readTag() to read the closing tag.
 *
 * This is important because if the tag includes sub-tags, you would end up
 * reading the closing tag every time. This happens when you read a sub-class
 * for example. Since we want the state to be the same in both cases, we
 * expect that extra r.readTag() call to happen.
 *
 * \param[in] name  The name of the field being read. In case of an array
 * the function may be called multiple times with the same name.
 * \param[in] r  The reader used to read the input data.
 */


} // namespace QtSerialization
// vim: ts=4 sw=4 et
