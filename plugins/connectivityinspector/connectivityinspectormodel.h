/*
  This file is part of GammaRay, the Qt application inspection and
  manipulation tool.

  Copyright (C) 2019 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Christian Gagneraud <chgans@gmail.com>

  Licensees holding valid commercial KDAB GammaRay licenses may use this file in
  accordance with GammaRay Commercial License Agreement provided with the Software.

  Contact info@kdab.com if any conditions of this licensing are not clear to you.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef GAMMARAY_CONNECTIVITYINSPECTOR_CONNECTIVITYINSECTORMODEL_H
#define GAMMARAY_CONNECTIVITYINSPECTOR_CONNECTIVITYINSECTORMODEL_H

#include <common/objectmodel.h>
#include <core/tools/objectinspector/outboundconnectionsmodel.h>

#include <QAbstractTableModel>
#include <QByteArray>
#include <QHash>
#include <QMetaMethod>

namespace GammaRay {

class Probe;

class ConnectionModel : public QAbstractTableModel
{
    Q_OBJECT

private:
    struct Edge;

public:
    enum { SenderColumn, ReceiverColumn, CountColumn, ColumnCount };
    enum {
        ObjectIdRole = ObjectModel::ObjectIdRole,
        ThreadIdRole = ObjectModel::UserRole + 1
    };

    explicit ConnectionModel(Probe *probe, QObject *parent = nullptr);
    ~ConnectionModel() override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    QMap<int, QVariant> itemData(const QModelIndex &index) const override;

public slots:
    void clear();
    void addOutboundConnection(QObject *sender, QObject *receiver);
    void removeOutboundConnections(QObject *sender);

private:
    // TODO: add timestamp + emit/invoke?
    struct Edge {
        Edge(QObject *sender, QObject *receiver, int value)
            : sender(sender)
            , receiver(receiver)
            , value(value)
        {}
        QObject *sender = nullptr;
        QObject *receiver = nullptr;
        int value = 0;
    };
    Probe *m_probe;
    QVector<Edge *> m_edges;
    // map[sender][receiver] = {sender, receiver, count}
    QHash<QObject *, QHash<QObject *, Edge *>> m_senderMap;
};
} // namespace GammaRay

#endif // GAMMARAY_CONNECTIVITYINSPECTOR_CONNECTIVITYINSECTORMODEL_H
