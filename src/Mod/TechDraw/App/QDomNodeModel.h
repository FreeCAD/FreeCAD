#ifndef _QDOMNODEMODEL_H_
#define _QDOMNODEMODEL_H_

#include <QAbstractXmlNodeModel>
#include <QXmlNamePool>
#include <QDomDocument>

class TechDrawExport QDomNodeModel: public QAbstractXmlNodeModel
{
public:
    QDomNodeModel(QXmlNamePool, QDomDocument, bool parsedReadOnly = false);
    QUrl baseUri ( const QXmlNodeModelIndex & n ) const;
    QXmlNodeModelIndex::DocumentOrder compareOrder ( const QXmlNodeModelIndex & ni1, const QXmlNodeModelIndex & ni2 ) const;
    QUrl documentUri ( const QXmlNodeModelIndex & n ) const;
    QXmlNodeModelIndex elementById ( const QXmlName & id ) const;
    QXmlNodeModelIndex::NodeKind kind ( const QXmlNodeModelIndex & ni ) const;
    QXmlName name ( const QXmlNodeModelIndex & ni ) const;
    QVector<QXmlName> namespaceBindings ( const QXmlNodeModelIndex & n ) const;
    QVector<QXmlNodeModelIndex> nodesByIdref ( const QXmlName & idref ) const;
    QXmlNodeModelIndex root ( const QXmlNodeModelIndex & n ) const;
    QSourceLocation    sourceLocation ( const QXmlNodeModelIndex & index ) const;
    QString    stringValue ( const QXmlNodeModelIndex & n ) const;
    QVariant typedValue ( const QXmlNodeModelIndex & node ) const;

public:
    QXmlNodeModelIndex fromDomNode (const QDomNode&) const;
    QDomNode toDomNode(const QXmlNodeModelIndex &) const;
    QVector<QDomNode> path(const QDomNode&) const;
    int childIndex(const QDomNode&) const;

protected:
    QVector<QXmlNodeModelIndex> attributes ( const QXmlNodeModelIndex & element ) const;
    QXmlNodeModelIndex nextFromSimpleAxis ( SimpleAxis axis, const QXmlNodeModelIndex & origin) const;

private:
    mutable QXmlNamePool m_Pool;
    mutable QDomDocument m_Doc;
    bool m_ReadOnly;
};

#endif // _QDOMNODEMODEL_H_
