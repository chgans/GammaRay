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

#include <QMutexLocker>
#include <QThread>

#include <algorithm>

using namespace GammaRay;

ConnectionModel::ConnectionModel(QObject *parent)
    : QAbstractTableModel(parent)
{}

ConnectionModel::~ConnectionModel() { qDeleteAll(m_items); }

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
    if (role == Qt::DisplayRole) {
        auto edge = m_items.at(index.row());
        switch (index.column()) {
        case SenderColumn:
            return edge->senderLabel;
        case ReceiverColumn:
            return edge->receiverLabel;
        case CountColumn:
            return edge->value;
        default:
            return {};
        }
    }
    if (role == ObjectIdRole) {
        auto edge = m_items.at(index.row());
        switch (index.column()) {
        case SenderColumn:
            return QVariant::fromValue(ObjectId(edge->sender));
        case ReceiverColumn:
            return QVariant::fromValue(ObjectId(edge->receiver));
        default:
            return {};
        }
    }
    if (role == ThreadIdRole) {
        auto edge = m_items.at(index.row());
        switch (index.column()) {
        case SenderColumn:
            return QVariant::fromValue(ObjectId(edge->senderThread));
        case ReceiverColumn:
            return QVariant::fromValue(ObjectId(edge->receiverThread));
        default:
            return {};
        }
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
    case SenderColumn:
        return "Sender";
    case ReceiverColumn:
        return "Receiver";
    case CountColumn:
        return "Connections";
    default:
        return {};
    }
}

QMap<int, QVariant> ConnectionModel::itemData(const QModelIndex &index) const
{
    QMap<int, QVariant> d = QAbstractItemModel::itemData(index);
    d.insert(ObjectIdRole, data(index, ObjectIdRole));
    d.insert(ThreadIdRole, data(index, ThreadIdRole));
    return d;
}

void ConnectionModel::clear()
{
    beginResetModel();
    qDeleteAll(m_items);
    m_items.clear();
    m_senderMap.clear();
    endResetModel();
}

bool ConnectionModel::hasConnection(QObject *sender, QObject *receiver) const {
    return m_senderMap.contains(sender) &&
        m_senderMap.value(sender).contains(receiver);
}

void ConnectionModel::addConnection(QObject *sender,
                                    QObject *senderThread,
                                    const QString &senderLabel,
                                    QObject *receiver,
                                    QObject *receiverThread,
                                    const QString &receiverLabel)
{
    if (sender == receiver)
        return;

    const bool update = m_senderMap.contains(sender)
                        && m_senderMap.value(sender).contains(receiver);
    qWarning() << __FUNCTION__ << update;
    if (update) {
        auto &edge = m_senderMap[sender][receiver];
        edge->value++;
        const int row = m_items.indexOf(edge);
        qWarning() << __FUNCTION__ << edge->value++;
        emit dataChanged(index(row, CountColumn), index(row, CountColumn));
    } else {
        const int row = m_items.count();
        beginInsertRows(QModelIndex(), row, row);
        auto edge = new ConnectionItem(sender,
                                       senderThread,
                                       senderLabel,
                                       receiver,
                                       receiverThread,
                                       receiverLabel,
                                       1);
        m_items.append(edge);
        m_senderMap[sender][receiver] = edge;
        endInsertRows();
    }
}

void ConnectionModel::removeConnection(QObject *sender, QObject *receiver) {
    if (!m_senderMap.contains(sender))
        return;
    if (!m_senderMap.value(sender).contains(receiver))
        return;
    const auto edge = m_senderMap.value(sender).value(receiver);
    Q_ASSERT(edge->sender == sender && edge->receiver == receiver);
    const int row = m_items.indexOf(edge);
    Q_ASSERT(row >= 0);
    beginRemoveRows(QModelIndex(), row, row);    
    m_items.removeAt(row);
    delete edge;
    m_senderMap[sender].remove(receiver);
    if (m_senderMap.value(sender).isEmpty())
        m_senderMap.remove(sender);
    endRemoveRows();
}

void ConnectionModel::removeSender(QObject *sender)
{
    for (auto edge : m_senderMap.value(sender).values()) {
        Q_ASSERT(edge->sender == sender);
        removeConnection(edge->sender, edge->receiver);
    }
    m_senderMap.remove(sender); // FIXME
}

bool ConnectionModel::hasSender(QObject *sender) const
{
    return m_senderMap.contains(sender) && !m_senderMap.value(sender).isEmpty();
}
