/*
Copyright (c) 2011, Stanislaw Adaszewski
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Stanislaw Adaszewski nor the
      names of other contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL STANISLAW ADASZEWSKI BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Copyright (c) 2012
Changes made by Alexis Wilke so the model works in Qt 4.8 and with Snap.
*/
#ifndef _QDOMNODEMODEL_H_
#define _QDOMNODEMODEL_H_

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#include <QAbstractXmlNodeModel>
#include <QXmlNamePool>
#include <QDomDocument>
#pragma GCC diagnostic pop

class QDomNodeModel: public QAbstractXmlNodeModel
{
public:
    typedef QVector<QDomNode> Path;

    QDomNodeModel(QXmlNamePool, QDomDocument);
    virtual ~QDomNodeModel() {}

    // QAbstractXmlNodeModel implementation
    virtual QUrl baseUri(const QXmlNodeModelIndex& n) const;
    virtual QXmlNodeModelIndex::DocumentOrder compareOrder(const QXmlNodeModelIndex& ni1,
                                                           const QXmlNodeModelIndex& ni2) const;
    virtual QUrl documentUri(const QXmlNodeModelIndex& n) const;
    virtual QXmlNodeModelIndex elementById(const QXmlName& id) const;
    virtual QXmlNodeModelIndex::NodeKind kind(const QXmlNodeModelIndex& ni) const;
    virtual QXmlName name(const QXmlNodeModelIndex& ni) const;
    virtual QVector<QXmlName> namespaceBindings(const QXmlNodeModelIndex& n) const;
    virtual QVector<QXmlNodeModelIndex> nodesByIdref(const QXmlName& idref) const;
    virtual QXmlNodeModelIndex root(const QXmlNodeModelIndex& n) const;
    virtual QString stringValue(const QXmlNodeModelIndex& n) const;
    virtual QVariant typedValue(const QXmlNodeModelIndex& node) const;

    //virtual QSourceLocation sourceLocation(const QXmlNodeModelIndex& index) const; -- this is not yet virtual

    QXmlNodeModelIndex fromDomNode(const QDomNode& n) const;
    QDomNode toDomNode(const QXmlNodeModelIndex& ni) const;

protected:
    // QAbstractXmlNodeModel implementation
    virtual QVector<QXmlNodeModelIndex> attributes(const QXmlNodeModelIndex& element) const;
    virtual QXmlNodeModelIndex nextFromSimpleAxis(SimpleAxis axis, const QXmlNodeModelIndex& origin) const;

private:
    Path path(const QDomNode& n) const;
    int childIndex(const QDomNode& n) const;

    mutable QXmlNamePool f_pool;
    mutable QDomDocument f_doc;
};

#endif // _QDOMNODEMODEL_H_
// vim: ts=4 sw=4 et
