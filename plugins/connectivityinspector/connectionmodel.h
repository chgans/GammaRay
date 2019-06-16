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

#ifndef GAMMARAY_CONNECTIVITYINSPECTOR_CONNECTIVITYINSPECTORMODEL_H
#define GAMMARAY_CONNECTIVITYINSPECTOR_CONNECTIVITYINSPECTORMODEL_H

#include <common/objectmodel.h>
#include <core/tools/objectinspector/outboundconnectionsmodel.h>

#include <QAbstractTableModel>
#include <QByteArray>
#include <QHash>
#include <QMetaMethod>

class QObjectPrivate;

namespace GammaRay {

class ConnectionModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum {
        ConnectionColumn,
        TypeColumn,
        SenderColumn,
        SignalColumn,
        ReceiverColumn,
        SlotColumn,
        ColumnCount
    };

    enum {
        ConnectionIdRole = Qt::UserRole + 1,
        SenderObjectIdRole,
        ReceiverObjectIdRole,
    };

    explicit ConnectionModel(QObject *parent = nullptr);
    ~ConnectionModel() override;

    quint64 id(int row)
    {
        Q_ASSERT(row >= 0 && row < m_items.count());
        return reinterpret_cast<quint64>(m_items.at(row).connection);
    }

    QObject *senderObject(int row)
    {
        Q_ASSERT(row >= 0 && row < m_items.count());
        return m_items.at(row).sender;
    }

    QObject *receiverObject(int row)
    {
        Q_ASSERT(row >= 0 && row < m_items.count());
        return m_items.at(row).receiver;
    }

public slots:
    void clear();
    void addObject(QObject *object);
    void removeObject(QObject *object);

    // QAbstractItemModel interface
public:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    QMap<int, QVariant> itemData(const QModelIndex &index) const override;

private:
    struct Connection
    {
        const void *connection;
        QObject *sender;
        int signalIndex;
        QObject *receiver;
        int slotIndex;
        int type;
        int count = 0;
    };
    QVector<Connection> m_items;
};
} // namespace GammaRay

#endif // GAMMARAY_CONNECTIVITYINSPECTOR_CONNECTIVITYINSPECTORMODEL_H
