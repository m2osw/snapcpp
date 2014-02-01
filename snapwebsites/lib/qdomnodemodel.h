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
