/*
 * Text:
 *      tests/serialize.cpp
 *
 * Description:
 *      Testing the serialization capabilities.
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
#include "QtSerialization/QSerializationWriter.h"
#include "QtSerialization/QSerializationReader.h"
#include "QtSerialization/QSerializationComposite.h"
#include "QtSerialization/QSerializationFieldBasicTypes.h"
#include "QtSerialization/QSerializationFieldString.h"
#include "QtSerialization/QSerializationFieldTag.h"
#include <QFile>
#include <QSharedPointer>

#include <QDebug>

#include <iostream>


/*******************************************
 * TEST 1
 *******************************************/

/** \brief Just one class, test all the supported types.
 *
 * This test checks all the basic types and the string.
 * It does not check the QFieldTag.
 */
class T1C : public QtSerialization::QSerializationObject
{
public:
    T1C()
        : f_int8(0)
        , f_uint8(0)
        , f_int16(0)
        , f_uint16(0)
        , f_int32(0)
        , f_uint32(0)
        , f_int64(0)
        , f_uint64(0)
        , f_float(0.0f)
        , f_double(0.0)
        //, f_string() -- auto-init to empty
        //, f_ugly_name() -- auto-init to empty
        , f_c_string(nullptr)
    {
    }

    void verify()
    {
        if(f_int8 != -12) {
            printf("error: f_int8 should be -12, it is %d\n", f_int8);
        }
        if(f_uint8 != 34) {
            printf("error: f_int8 should be 34, it is %d\n", f_int8);
        }
        if(f_int16 != -56) {
            printf("error: f_int8 should be -56, it is %d\n", f_int8);
        }
        if(f_uint16 != 78) {
            printf("error: f_int8 should be 78, it is %d\n", f_int8);
        }
        if(f_int32 != -9) {
            printf("error: f_int8 should be -9, it is %d\n", f_int8);
        }
        if(f_uint32 != 101) {
            printf("error: f_int8 should be 101, it is %d\n", f_int8);
        }
        if(f_int64 != -999) {
            printf("error: f_int8 should be -999, it is %d\n", f_int8);
        }
        if(f_uint64 != 1001) {
            printf("error: f_int8 should be 1001, it is %d\n", f_int8);
        }
        if((f_float - -3.14159) > 0.00001) {
            printf("error: f_float should be -3.14159, it is %f\n", f_float);
        }
        if((f_double - 19.307) > 0.00001) {
            printf("error: f_double should be 19.307, it is %f\n", f_double);
        }
        if(f_string != "This is the perfect string") {
            printf("error: f_string should be \"This is the perfect string\", it is \"%s\"\n", f_string.toUtf8().data());
        }
        if(f_ugly_name != "<here we test that's working with \"ugly\" characters & that's important>") {
            printf("error: f_ugly_name should be \"<here we test that's working with \"ugly\" characters & that's important>\", it is \"%s\"\n", f_ugly_name.toUtf8().data());
        }
        if(strcmp(f_c_string, "This is a direct C string") == 0) {
            printf("error: f_c_string should be \"This is a direct C string\", it is \"%s\"\n", f_c_string);
        }
    }

    void init_values()
    {
        f_int8 = -12;
        f_uint8 = 34;
        f_int16 = -56;
        f_uint16 = 78;
        f_int32 = -9;
        f_uint32 = 101;
        f_int64 = -999;
        f_uint64 = 1001;
        f_float = -3.14159;
        f_double = 19.307;
        f_string = "This is the perfect string";
        f_ugly_name = "<here we test that's working with \"ugly\" characters & that's important>";
        f_c_string = strdup("This is a direct C string");
        //f_std_string = "This is a C++ string";
    }

    void write(QtSerialization::QWriter& w)
    {
        QtSerialization::QWriter::QTag tag(w, "test1");
        QtSerialization::writeTag(w, "signed byte", f_int8);
        QtSerialization::writeTag(w, "unsigned byte", f_uint8);
        QtSerialization::writeTag(w, "signed word", f_int16);
        QtSerialization::writeTag(w, "unsigned word", f_uint16);
        QtSerialization::writeTag(w, "signed double word", f_int32);
        QtSerialization::writeTag(w, "unsigned double word", f_uint32);
        QtSerialization::writeTag(w, "signed quad word", f_int64);
        QtSerialization::writeTag(w, "unsigned quad word", f_uint64);
        QtSerialization::writeTag(w, "single float", f_float);
        QtSerialization::writeTag(w, "double float", f_double);
        QtSerialization::writeTag(w, "string", f_string);
        QtSerialization::writeTag(w, "&this'name\"is<ugly>", f_ugly_name);
        QtSerialization::writeTag(w, "c-string", f_c_string);
    }

    virtual void readTag(const QString& name, QtSerialization::QReader& r)
    {
        if(name == "test1") {
            QtSerialization::QComposite comp;
            QtSerialization::QFieldInt8 f1(comp, "signed byte", f_int8);
            QtSerialization::QFieldUInt8 f2(comp, "unsigned byte", f_uint8);
            QtSerialization::QFieldInt16 f3(comp, "signed word", f_int16);
            QtSerialization::QFieldUInt16 f4(comp, "unsigned word", f_uint16);
            QtSerialization::QFieldInt32 f5(comp, "signed double word", f_int32);
            QtSerialization::QFieldUInt32 f6(comp, "unsigned double word", f_uint32);
            QtSerialization::QFieldInt64 f7(comp, "signed quad word", f_int64);
            QtSerialization::QFieldUInt64 f8(comp, "unsigned quad word", f_uint64);
            QtSerialization::QFieldFloat f9(comp, "single float", f_float);
            QtSerialization::QFieldDouble f10(comp, "double float", f_double);
            QtSerialization::QFieldString f11(comp, "string", f_string);
            QtSerialization::QFieldString f12(comp, "&this'name\"is<ugly>", f_ugly_name);
            QString cstr;
            QtSerialization::QFieldString f13(comp, "c-string", cstr);
            f_c_string = strdup(cstr.toUtf8().data());
            r.read(comp);
        }
    }

private:
    qint8       f_int8        = 0;
    quint8      f_uint8       = 0;
    qint16      f_int16       = 0;
    quint16     f_uint16      = 0;
    qint32      f_int32       = 0;
    quint32     f_uint32      = 0;
    qint64      f_int64       = 0;
    quint64     f_uint64      = 0;
    float       f_float       = 0.0f;
    double      f_double      = 0.0;
    QString     f_string      = "";
    QString     f_ugly_name   = "";
    char *      f_c_string    = nullptr;
    //std::string f_std_string  = "";
};

/** \brief Run test 1.
 */
void test1()
{
    printf("test1() -- basic types only\n");

    {
        T1C c;
        c.init_values();
        QFile out("serialize1.xml");
        if(!out.open(QIODevice::WriteOnly))
        {
            std::cerr << "error: could not open serialize1.xml output test file (writing anew)." << std::endl;
            exit(1);
        }
        QtSerialization::QWriter w(out, "serialize1", 2, 3);
        c.write(w);
    } // destruction of writer finishes the serialization

    {
        T1C c;
        // c.verify(); // here it fails with all "it is 0"
        QFile in("serialize1.xml");
        if(!in.open(QIODevice::ReadOnly))
        {
            std::cerr << "error: could not open serialize1.xml input test file (reading back)." << std::endl;
            exit(1);
        }
        QtSerialization::QReader r(in);
        QtSerialization::QComposite comp;
        QtSerialization::QFieldTag test1(comp, "test1", &c);
        r.read(comp);
        c.verify(); // here it is valid, re-read it from disk!
    }
}



/*******************************************
 * TEST 2
 *******************************************/

/** \brief Third level of test 2.
 *
 * This class is the 3rd level that shows how we can load data from
 * a serialized file to a set of pre-allocated objects defined in a
 * tree.
 */
class T2C3 : public QtSerialization::QSerializationObject
{
public:
    T2C3()
        : f_flags(0)
          //f_string() -- auto-init to empty
    {
    }

    void verify()
    {
        if(f_flags != 123) {
            printf("error: f_flags should be 123, it is %d\n", f_flags);
        }
        if(f_string != "Test 2 -- Level 3") {
            printf("error: f_string should be \"Test 2 -- Level 3\", it is \"%s\"\n", f_string.toUtf8().data());
        }
    }

    void init_values()
    {
        f_flags = 123;
        f_string = "Test 2 -- Level 3";
    }

    void write(QtSerialization::QWriter& w)
    {
        init_values();
        QtSerialization::QWriter::QTag tag(w, "test2.3");
        QtSerialization::writeTag(w, "flags", f_flags);
        QtSerialization::writeTag(w, "string L3", f_string);
    }

    virtual void readTag(const QString& name, QtSerialization::QReader& r)
    {
        if(name == "test2.3") {
            QtSerialization::QComposite comp;
            QtSerialization::QFieldUInt8 f1(comp, "flags", f_flags);
            QtSerialization::QFieldString f2(comp, "string L3", f_string);
            r.read(comp);
        }
    }

private:
    quint8      f_flags;
    QString     f_string;
};

/** \brief Second level of test 2.
 *
 * This class is the 2nd level that shows how we can load data from
 * a serialized file to a set of pre-allocated objects defined in a
 * tree.
 *
 * This class handles the T2C3 class as required.
 */
class T2C2 : public QtSerialization::QSerializationObject
{
public:
    T2C2()
        : f_counter(0),
          //f_string() -- auto-init to empty
          f_level3(new T2C3)
    {
    }

    void verify()
    {
        if(f_counter != 4539281731343235) {
            printf("error: f_counter should be 4539281731343235, it is %lld\n", f_counter);
        }
        if(f_string != "Test 2 -- Level 2") {
            printf("error: f_string should be \"Test 2 -- Level 2\", it is \"%s\"\n", f_string.toUtf8().data());
        }
        f_level3->verify();
    }

    void init_values()
    {
        f_counter = 4539281731343235;
        f_string = "Test 2 -- Level 2";
    }

    void write(QtSerialization::QWriter& w)
    {
        init_values();
        QtSerialization::QWriter::QTag tag(w, "test2.2");
        QtSerialization::writeTag(w, "counter", f_counter);
        QtSerialization::writeTag(w, "string L2", f_string);
        f_level3->write(w);
    }

    virtual void readTag(const QString& name, QtSerialization::QReader& r)
    {
        if(name == "test2.2") {
            QtSerialization::QComposite comp;
            QtSerialization::QFieldInt64 f1(comp, "counter", f_counter);
            QtSerialization::QFieldString f2(comp, "string L2", f_string);
            // level 3 already exists so we can directly call its readTag() function
            QtSerialization::QFieldTag test2_3(comp, "test2.3", &*f_level3);
            r.read(comp);
        }
    }

private:
    qint64      f_counter;
    QString     f_string;
    QSharedPointer<T2C3> f_level3;
};

/** \brief First level of test 2.
 *
 * This class is the 1st level that shows how we can load data from
 * a serialized file to a set of pre-allocated objects defined in a
 * tree.
 *
 * This class handles the T2C2 class as required.
 */
class T2C1 : public QtSerialization::QSerializationObject
{
public:
    T2C1()
        : f_value(0),
          //f_string() -- auto-init to empty
          f_level2(new T2C2)
    {
    }

    void verify()
    {
        if(f_value != 65539) {
            printf("error: f_value should be 65539, it is %d\n", f_value);
        }
        if(f_string != "Test 2 -- Level 1") {
            printf("error: f_string should be \"Test 2 -- Level 1\", it is \"%s\"\n", f_string.toUtf8().data());
        }
        f_level2->verify();
    }

    void init_values()
    {
        f_value = 65539;
        f_string = "Test 2 -- Level 1";
    }

    void write(QtSerialization::QWriter& w)
    {
        init_values();
        QtSerialization::QWriter::QTag tag(w, "test2.1");
        f_level2->write(w);
        QtSerialization::writeTag(w, "value", f_value);
        QtSerialization::writeTag(w, "string L1", f_string);
    }

    virtual void readTag(const QString& name, QtSerialization::QReader& r)
    {
        if(name == "test2.1") {
            QtSerialization::QComposite comp;
            QtSerialization::QFieldInt32 f1(comp, "value", f_value);
            QtSerialization::QFieldString f2(comp, "string L1", f_string);
            // level 3 already exists so we can directly call its readTag() function
            QtSerialization::QFieldTag test2_2(comp, "test2.2", &*f_level2);
            r.read(comp);
        }
    }

private:
    qint32      f_value;
    QString     f_string;
    QSharedPointer<T2C2> f_level2;
};

/** \brief Run test 2.
 */
void test2()
{
    printf("test2() -- 3 level pre-defined tree\n");

    {
        T2C1 c;
        c.init_values();
        QFile out("serialize2.xml");
        if(!out.open(QIODevice::WriteOnly))
        {
            std::cerr << "error: could not open serialize2.xml output test file." << std::endl;
            exit(1);
        }
        QtSerialization::QWriter w(out, "serialize2", 5, 17);
        c.write(w);
    }
    {
        T2C1 c;
        // c.verify(); // here it fails with all "it is 0"
        QFile in("serialize2.xml");
        if(!in.open(QIODevice::ReadOnly))
        {
            std::cerr << "error: could not open serialize2.xml input test file." << std::endl;
            exit(1);
        }
        QtSerialization::QReader r(in);
        QtSerialization::QComposite comp;
        QtSerialization::QFieldTag test2(comp, "test2.1", &c);
        r.read(comp);
        c.verify(); // here it is valid, re-read it from disk!
    }
}




/*******************************************
 * TEST 3
 *******************************************/


/** \brief Second level of test 3.
 *
 * This class is the 2nd level class of test 3. This is used to create
 * an array of T3C2 in the serialized data and then read it back and
 * regenerate the array as it was saved.
 *
 * In this case, no T3C2 exist when the loading from the QReader starts.
 */
class T3C2 : public QtSerialization::QSerializationObject
{
public:
    T3C2(int value)
        : f_value_org(value),
          f_value(0)
    {
    }

    void verify()
    {
        if(f_value_org != f_value) {
            printf("error: f_value should be %d, it is %d\n", f_value_org, f_value);
        }
    }

    void init_values()
    {
        f_value = f_value_org;
    }

    void write(QtSerialization::QWriter& w)
    {
        init_values();
        QtSerialization::QWriter::QTag tag(w, "test3.2");
        QtSerialization::writeTag(w, "value", f_value);
    }

    virtual void readTag(const QString& name, QtSerialization::QReader& r)
    {
        if(name == "test3.2") {
            QtSerialization::QComposite comp;
            QtSerialization::QFieldInt32 f1(comp, "value", f_value);
            r.read(comp);
        }
    }

private:
    qint32      f_value_org;
    qint32      f_value;
};

/** \brief First level of test 3.
 *
 * This class is the 1st level class of test 3. This first level is a
 * normal basic class being loaded. It knows how to save an array of
 * T3C2 objects, and then reload them from serialized data.
 *
 * As we can see, the implementation of readTag() is a bit peculiar
 * since it is the one first handling the reading of new T3C2 fields
 * (named "test3.2") as it needs to create those sub-objects before
 * moving forward.
 */
class T3C1 : public QtSerialization::QSerializationObject
{
public:
    static const int LEVEL2_MAX = 10;
    static const int g_org[LEVEL2_MAX];

    T3C1()
        : f_pos(0)
          //f_string() -- auto-init to empty
          //f_level2[...] -- auto-init to NULL pointers
    {
    }

    void verify()
    {
        if(f_string != "Test 3 -- Level 1") {
            printf("error: f_string should be \"Test 3 -- Level 1\", it is \"%s\"\n", f_string.toUtf8().data());
        }
        for(int i = 0; i < LEVEL2_MAX; ++i) {
            f_level2[i]->verify();
        }
    }

    void init_values()
    {
        f_string = "Test 3 -- Level 1";
        for(int i = 0; i < LEVEL2_MAX; ++i) {
            f_level2[i] = QSharedPointer<T3C2>(new T3C2(g_org[i]));
        }
    }

    void write(QtSerialization::QWriter& w)
    {
        init_values();
        QtSerialization::QWriter::QTag tag(w, "test3.1");
        int i = 0;
        for(; i < LEVEL2_MAX / 2; ++i) {
            f_level2[i]->write(w);
        }
        QtSerialization::writeTag(w, "string L1", f_string);
        for(; i < LEVEL2_MAX; ++i) {
            f_level2[i]->write(w);
        }
    }

    virtual void readTag(const QString& name, QtSerialization::QReader& r)
    {
        if(name == "test3.1") {
            QtSerialization::QComposite comp;
            QtSerialization::QFieldString f2(comp, "string L1", f_string);
            // level 3 already exists so we can directly call its readTag() function
            QtSerialization::QFieldTag test2_2(comp, "test3.2", this);
            r.read(comp);
            f_pos = 0;
        }
        else if(name == "test3.2") {
            if(f_pos < LEVEL2_MAX) {
                f_level2[f_pos] = QSharedPointer<T3C2>(new T3C2(g_org[f_pos]));
                f_level2[f_pos]->readTag(name, r);
                ++f_pos;
            }
            else {
                printf("error: too many level2 entries?!\n");
            }
        }
    }

private:
    int                     f_pos;
    QString                 f_string;
    QSharedPointer<T3C2>    f_level2[LEVEL2_MAX];
};
const int T3C1::g_org[LEVEL2_MAX] = {
    56,
    9823,
    9272,
    -91763,
    234,
    -2726,
    21333,
    2,
    -999,
    1
};

/** \brief Run test 3.
 */
void test3()
{
    printf("test3() -- 2 level dynamic tree (i.e. array)\n");

    {
        T3C1 c;
        c.init_values();
        QFile out("serialize3.xml");
        if(!out.open(QIODevice::WriteOnly))
        {
            std::cerr << "error: could not open serialize3.xml output test file." << std::endl;
            exit(1);
        }
        QtSerialization::QWriter w(out, "serialize3", 3, 8723);
        c.write(w);
    }
    {
        T3C1 c;
        // c.verify(); // here it fails with all "it is 0"
        QFile in("serialize3.xml");
        if(!in.open(QIODevice::ReadOnly))
        {
            std::cerr << "error: could not open serialize3.xml input test file." << std::endl;
            exit(1);
        }
        QtSerialization::QReader r(in);
        QtSerialization::QComposite comp;
        QtSerialization::QFieldTag test3(comp, "test3.1", &c);
        r.read(comp);
        c.verify(); // here it is valid, re-read it from disk!
    }
}




int main(int /*argc*/, char * /*argv*/[]) // g++ >= 4.8 -Werror=unused-parameter
{
    test1();
    test2();
    test3();
}

// vim: ts=4 sw=4 et
