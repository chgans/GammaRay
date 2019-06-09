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

#include "connectionmodel.h"

#include <core/probe.h>
#include <core/tools/objectinspector/outboundconnectionsmodel.h>
#include <core/util.h>

#include <common/objectid.h>

#include <private/qobject_p.h>
#include <QMutexLocker>
#include <QThread>

#include <algorithm>

namespace {
int signalIndexToMethodIndex(QObject *object, int signalIndex)
{
    if (signalIndex < 0)
        return signalIndex;
    Q_ASSERT(object);

    return GammaRay::Util::signalIndexToMethodIndex(object->metaObject(), signalIndex);
}
} // namespace

using namespace GammaRay;

ConnectionModel::ConnectionModel(QObject *parent)
    : QAbstractTableModel(parent)
{}

ConnectionModel::~ConnectionModel() = default;

int ConnectionModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_items.count();
}

int ConnectionModel::columnCount(const QModelIndex & /*parent*/) const
{
    return ColumnCount;
}

QVariant ConnectionModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    Q_ASSERT(index.row() < m_items.count());
    auto item = m_items.at(index.row());
    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case ConnectionColumn:
            return Util::addressToString(item.connection);
        case TypeColumn:
            return item.type;
        case SenderColumn:
            return Util::addressToString(item.sender);
        case SignalColumn:
            return item.signalIndex;
        case ReceiverColumn:
            return Util::addressToString(item.receiver);
        case SlotColumn:
            return item.slotIndex;
        default:
            return {};
        }
    }
    if (role == ConnectionIdRole) {
        auto id = reinterpret_cast<QObject *>(const_cast<void *>(item.connection));
        return QVariant::fromValue(ObjectId(id));
    }
    if (role == SenderObjectIdRole) {
        return QVariant::fromValue(ObjectId(item.sender));
    }
    if (role == ReceiverObjectIdRole) {
        return QVariant::fromValue(ObjectId(item.receiver));
    }
    return {};
}

QVariant ConnectionModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return {};
    if (orientation != Qt::Horizontal)
        return {};
    switch (section) {
    case ConnectionColumn:
        return tr("Connection");
    case TypeColumn:
        return tr("Type");
    case SenderColumn:
        return tr("Sender");
    case SignalColumn:
        return tr("Signal");
    case ReceiverColumn:
        return tr("Receiver");
    case SlotColumn:
        return tr("Slot");
    default:
        return {};
    }
}

QMap<int, QVariant> ConnectionModel::itemData(const QModelIndex &index) const
{
    if (!index.isValid())
        return {};
    QMap<int, QVariant> map = QAbstractTableModel::itemData(index);
    map.insert(ConnectionIdRole, this->data(index, ConnectionIdRole));
    map.insert(SenderObjectIdRole, this->data(index, SenderObjectIdRole));
    map.insert(ReceiverObjectIdRole, this->data(index, ReceiverObjectIdRole));
    return map;
}

void ConnectionModel::clear()
{
    beginResetModel();
    m_items.clear();
    endResetModel();
}

void ConnectionModel::addObject(QObject *object)
{
    if (Probe::instance()->filterObject(object))
        return;
    QObjectPrivate *d = QObjectPrivate::get(object);
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QObjectPrivate::ConnectionData *cd = d->connections.load();
    if (cd) {
        auto &cl = *(cd->signalVector.load());
#else
    if (d->connectionLists) {
        // HACK: the declaration of d->connectionsLists is not accessible for us...
        const auto &cl = *reinterpret_cast<QVector<QObjectPrivate::ConnectionList> *>(
            d->connectionLists);
#endif
        for (int signalIndex = 0; signalIndex < cl.count(); ++signalIndex) {
            const QObjectPrivate::Connection *c = cl.at(signalIndex).first;
            while (c) {
                if (!c->receiver || Probe::instance()->filterObject(c->receiver)) {
                    c = c->nextConnectionList;
                    continue;
                }
                Connection conn;
                conn.connection = c;
                conn.sender = object;
                conn.receiver = c->receiver;
                conn.signalIndex = signalIndexToMethodIndex(object, signalIndex);
                if (c->isSlotObject)
                    conn.slotIndex = -1;
                else
                    conn.slotIndex = c->method();
                conn.type = c->connectionType;
                c = c->nextConnectionList;
                const int row = m_items.count();
                beginInsertRows(QModelIndex(), row, row);
                m_items.append(conn);
                endInsertRows();
            }
        }
    }
}

void ConnectionModel::removeObject(QObject *object)
{
    for (int row = 0; row < m_items.count(); ++row) {
        const auto &connection = m_items.at(row);
        if (connection.sender != object && connection.receiver != object)
            continue;
        beginRemoveRows(QModelIndex(), row, row);
        m_items.removeAt(row);
        endRemoveRows();
    }
}
