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

ConnectionModel::ConnectionModel(Probe *probe, QObject *parent)
    : QAbstractTableModel(parent)
    , m_probe(probe)
{}

ConnectionModel::~ConnectionModel() { qDeleteAll(m_items); }

int ConnectionModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    QMutexLocker lock(m_probe->objectLock());
    return m_items.count();
}

int ConnectionModel::columnCount(const QModelIndex & /*parent*/) const
{
    return ColumnCount;
}

QVariant ConnectionModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole) {
        QMutexLocker lock(m_probe->objectLock());
        auto edge = m_items.at(index.row());
        switch (index.column()) {
        case SenderColumn:
            if (m_probe->isValidObject(edge->sender))
                return Util::shortDisplayString(edge->sender);
            return Util::addressToString(edge->sender);
        case ReceiverColumn:
            if (m_probe->isValidObject(edge->receiver))
                return Util::shortDisplayString(edge->receiver);
            return Util::addressToString(edge->receiver);
        case CountColumn:
            return edge->value;
        default:
            return {};
        }
    }
    if (role == ObjectIdRole) {
        QMutexLocker lock(m_probe->objectLock());
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
        QMutexLocker lock(m_probe->objectLock());
        auto edge = m_items.at(index.row());
        switch (index.column()) {
        case SenderColumn:
            if (m_probe->isValidObject(edge->sender))
                return QVariant::fromValue(ObjectId(edge->sender->thread()));
            return QVariant::fromValue(ObjectId());
        case ReceiverColumn:
            if (m_probe->isValidObject(edge->receiver))
                return QVariant::fromValue(ObjectId(edge->receiver->thread()));
            return QVariant::fromValue(ObjectId());
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

void ConnectionModel::addConnection(QObject *sender, QObject *receiver) {
    if (sender == receiver)
        return;

    const bool update = m_senderMap.contains(sender) && m_senderMap[sender].contains(receiver);
    if (update) {
        auto &edge = m_senderMap[sender][receiver];
        edge->value++;
        const int row = m_items.indexOf(edge);
        emit dataChanged(index(row, CountColumn), index(row, CountColumn));
    } else {
        const int row = m_items.count();
        beginInsertRows(QModelIndex(), row, row);
        auto edge = new ConnectionItem(sender, receiver, 1);
        m_items.append(edge);
        m_senderMap[sender][receiver] = edge;
        endInsertRows();
    }
}

void ConnectionModel::removeConnections(QObject *sender) {
    for (auto edge : m_senderMap.value(sender).values()) {
        const auto row = m_items.indexOf(edge);
        beginRemoveRows(QModelIndex(), row, row);
        m_items.removeAt(row);
        delete edge;
        endRemoveRows();
    }
    m_senderMap.remove(sender); // FIXME
}

void ConnectionModel::removeConnection(QObject *sender, QObject *receiver) {
    if (!m_senderMap.contains(sender))
        return;
    if (!m_senderMap.value(sender).contains(receiver))
        return;
    const auto edge = m_senderMap.value(sender).value(receiver);
    const int row = m_items.indexOf(edge);
    beginRemoveRows(QModelIndex(), row, row);
    qDeleteAll(m_senderMap[sender].values());
    m_items.removeAt(row);
    m_senderMap[sender].remove(receiver);
    endRemoveRows();
}