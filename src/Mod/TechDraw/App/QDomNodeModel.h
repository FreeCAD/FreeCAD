//  Copyright (c) 2011 Stanislaw Adaszewski, portions (c) 2019 Tomas Pavlicek
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//      * Redistributions of source code must retain the above copyright
//        notice, this list of conditions and the following disclaimer.
//      * Redistributions in binary form must reproduce the above copyright
//        notice, this list of conditions and the following disclaimer in the
//        documentation and/or other materials provided with the distribution.
//      * Neither the name of Stanislaw Adaszewski nor the
//        names of other contributors may be used to endorse or promote products
//        derived from this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
//  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//  DISCLAIMED. IN NO EVENT SHALL STANISLAW ADASZEWSKI BE LIABLE FOR ANY
//  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
//  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
//  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
//  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//  * Used under BSD license 2.0 *

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
