/*
 * Header:
 *      include/QtSerialization/QSerializationFieldBasicTypes.h
 *
 * Description:
 *      Implementation of the basic types (integers, floats, etc.) for
 *      QReader objects.
 *
 * Documentation:
 *      See below.
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
#ifndef QT_SERIALIZATION_BASIC_TYPES_H
#define QT_SERIALIZATION_BASIC_TYPES_H

#include "QSerializationField.h"
#include "QSerializationReader.h"
#include "QSerializationExceptions.h"

namespace QtSerialization
{

/** \brief Convert basic data from a string to a field.
 *
 * This template is used to allow data conversion from many different
 * C++ types to a type supported by a user class field.
 *
 * \param[out] field  The reference to the field where the converted data is saved.
 * \param[in] data  The data to convert and save in \p field.
 */
template<class T>
void convertData(T& field, const QString& data);

/** \brief Convert basic data from a string to a field of type bool.
 *
 * This template specialization is used to convert a string to a field
 * of type bool defined in the user class.
 *
 * A boolean is set to true if the read value is not zero and it is set to false
 * when the value is zero.
 *
 * \param[out] field  The reference to the field where the converted data is saved.
 * \param[in] data  The data to convert and save in \p field.
 */
template<>
inline void convertData<bool>(bool& field, const QString& data)
{
    field = data.toInt() != 0;
}

/** \brief Convert basic data from a string to a field of type qint8.
 *
 * This template specialization is used to convert a string to a field
 * of type qint8 defined in the user class.
 *
 * \param[out] field  The reference to the field where the converted data is saved.
 * \param[in] data  The data to convert and save in \p field.
 */
template<>
inline void convertData<qint8>(qint8& field, const QString& data)
{
    field = static_cast<qint8>(data.toShort());
}

/** \brief Convert basic data from a string to a field of type quint8.
 *
 * This template specialization is used to convert a string to a field
 * of type quint8 defined in the user class.
 *
 * \param[out] field  The reference to the field where the converted data is saved.
 * \param[in] data  The data to convert and save in \p field.
 */
template<>
inline void convertData<quint8>(quint8& field, const QString& data)
{
    field = static_cast<quint8>(data.toUShort());
}

/** \brief Convert basic data from a string to a field of type qint16.
 *
 * This template specialization is used to convert a string to a field
 * of type qint16 defined in the user class.
 *
 * \param[out] field  The reference to the field where the converted data is saved.
 * \param[in] data  The data to convert and save in \p field.
 */
template<>
inline void convertData<qint16>(qint16& field, const QString& data)
{
    field = data.toShort();
}

/** \brief Convert basic data from a string to a field of type quint16.
 *
 * This template specialization is used to convert a string to a field
 * of type quint16 defined in the user class.
 *
 * \param[out] field  The reference to the field where the converted data is saved.
 * \param[in] data  The data to convert and save in \p field.
 */
template<>
inline void convertData<quint16>(quint16& field, const QString& data)
{
    field = data.toUShort();
}

/** \brief Convert basic data from a string to a field of type qint32.
 *
 * This template specialization is used to convert a string to a field
 * of type qint32 defined in the user class.
 *
 * \param[out] field  The reference to the field where the converted data is saved.
 * \param[in] data  The data to convert and save in \p field.
 */
template<>
inline void convertData<qint32>(qint32& field, const QString& data)
{
    field = data.toInt();
}

/** \brief Convert basic data from a string to a field of type quint32.
 *
 * This template specialization is used to convert a string to a field
 * of type quint32 defined in the user class.
 *
 * \param[out] field  The reference to the field where the converted data is saved.
 * \param[in] data  The data to convert and save in \p field.
 */
template<>
inline void convertData<quint32>(quint32& field, const QString& data)
{
    field = data.toUInt();
}

/** \brief Convert basic data from a string to a field of type qint64.
 *
 * This template specialization is used to convert a string to a field
 * of type qint64 defined in the user class.
 *
 * \param[out] field  The reference to the field where the converted data is saved.
 * \param[in] data  The data to convert and save in \p field.
 */
template<>
inline void convertData<qint64>(qint64& field, const QString& data)
{
    field = data.toLongLong();
}

/** \brief Convert basic data from a string to a field of type quint64.
 *
 * This template specialization is used to convert a string to a field
 * of type quint64 defined in the user class.
 *
 * \param[out] field  The reference to the field where the converted data is saved.
 * \param[in] data  The data to convert and save in \p field.
 */
template<>
inline void convertData<quint64>(quint64& field, const QString& data)
{
    field = data.toULongLong();
}

/** \brief Convert basic data from a string to a field of type float.
 *
 * This template specialization is used to convert a string to a field
 * of type float defined in the user class.
 *
 * \param[out] field  The reference to the field where the converted data is saved.
 * \param[in] data  The data to convert and save in \p field.
 */
template<>
inline void convertData<float>(float& field, const QString& data)
{
    field = data.toFloat();
}

/** \brief Convert basic data from a string to a field of type double.
 *
 * This template specialization is used to convert a string to a field
 * of type double defined in the user class.
 *
 * \param[out] field  The reference to the field where the converted data is saved.
 * \param[in] data  The data to convert and save in \p field.
 */
template<>
inline void convertData<double>(double& field, const QString& data)
{
    field = data.toDouble();
}


/** \brief Template handling all C++ basic types.
 *
 * This template class is used to handle all the C++ basic types.
 *
 * At this point we support the following types:
 *
 * \li bool
 * \li qint8
 * \li quint8
 * \li qint16
 * \li quint16
 * \li qint32
 * \li quint32
 * \li qint64
 * \li quint64
 * \li float
 * \li double
 *
 * Other basic types may be added as time passes. Some systems
 * offer char, wchar_t, long double, and size_t as a separate
 * types. With time, we certainly want to provide those too.
 */
template<class T>
class QFieldBasicType : public QField
{
public:
    /** \brief Initializes a basic type template.
     *
     * This constructor creates a field reference of type \p T.
     * The constructor takes the parent composite, the name of
     * the field as it appears in the 'n' attribute of the \<v>
     * tag and a reference to a user field of type \p T.
     *
     * The field being referenced must remain valid for the whole
     * time this templated object is available. The data read
     * from the serialized data is directly saved in that field.
     *
     * Remember that field names have to be unique. To create an
     * array of fields with the same name, you have to use the
     * QFieldTag instead.
     *
     * \param[in] composite  The parent composite where the field is added.
     * \param[in] name  The name of this field.
     * \param[in] field  A reference to the field where the data read is saved.
     */
    QFieldBasicType<T>(QComposite& composite, const QString& name, T& field)
    	: QField(composite, name),
          f_field(field)  // keep a reference
    {
        if(&f_field == NULL) {
            throw QExceptionNullReference("a basic type reference cannot be a NULL pointer");
        }
    }

    /** \brief Implementation of the read() function.
     *
     * This function is called whenever the field with the name as specified
     * in the constructor is read by the reader.
     *
     * This function reads the text representing the field, then calls the
     * convertData<T>() function corresponding to the type of field being
     * read. Finally it calls r.readTag() to read the closing \</v> tag.
     */
    virtual void read(QReader& r)
    {
        r.readText();
        convertData<T>(f_field, r.text());
        // also read the closing tag so it's equivalent to a composite field
        r.readTag();
    }

private:
    T& f_field;
};

/** \brief Specialization of the basic type bool.
 *
 * This typedef can be used as the specialized basic type template of
 * type bool.
 */
typedef QFieldBasicType<bool>        QFieldBool;

/** \brief Specialization of the basic type qint8.
 *
 * This typedef can be used as the specialized basic type template of
 * type qint8.
 */
typedef QFieldBasicType<qint8>        QFieldInt8;

/** \brief Specialization of the basic type quint8.
 *
 * This typedef can be used as the specialized basic type template of
 * type quint8.
 */
typedef QFieldBasicType<quint8>       QFieldUInt8;

/** \brief Specialization of the basic type qint16.
 *
 * This typedef can be used as the specialized basic type template of
 * type qint16.
 */
typedef QFieldBasicType<qint16>       QFieldInt16;

/** \brief Specialization of the basic type quint16.
 *
 * This typedef can be used as the specialized basic type template of
 * type quint16.
 */
typedef QFieldBasicType<quint16>      QFieldUInt16;

/** \brief Specialization of the basic type qint32.
 *
 * This typedef can be used as the specialized basic type template of
 * type qint32.
 */
typedef QFieldBasicType<qint32>       QFieldInt32;

/** \brief Specialization of the basic type quint32.
 *
 * This typedef can be used as the specialized basic type template of
 * type quint32.
 */
typedef QFieldBasicType<quint32>      QFieldUInt32;

/** \brief Specialization of the basic type qint64.
 *
 * This typedef can be used as the specialized basic type template of
 * type qint64.
 */
typedef QFieldBasicType<qint64>       QFieldInt64;

/** \brief Specialization of the basic type quint64.
 *
 * This typedef can be used as the specialized basic type template of
 * type quint64.
 */
typedef QFieldBasicType<quint64>      QFieldUInt64;

/** \brief Specialization of the basic type float.
 *
 * This typedef can be used as the specialized basic type template of
 * type float.
 */
typedef QFieldBasicType<float>        QFieldFloat;

/** \brief Specialization of the basic type double.
 *
 * This typedef can be used as the specialized basic type template of
 * type double.
 */
typedef QFieldBasicType<double>       QFieldDouble;




} // namespace QtSerialization
#endif
// #ifndef QT_SERIALIZATION_BASIC_TYPES_H
// vim: ts=4 sw=4 et
