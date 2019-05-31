/*
  objectvisualizermodel.cpp

  This file is part of GammaRay, the Qt application inspection and
  manipulation tool.

  Copyright (C) 2013-2019 Klarälvdalens Datakonsult AB, a KDAB Group company,
  info@kdab.com Author: Volker Krause <volker.krause@kdab.com>

  Licensees holding valid commercial KDAB GammaRay licenses may use this file in
  accordance with GammaRay Commercial License Agreement provided with the
  Software.

  Contact info@kdab.com if any conditions of this licensing are not clear to
  you.

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

#include "objectvisualizermodel.h"

#include <core/probe.h>
#include <core/tools/objectinspector/inboundconnectionsmodel.h>
#include <core/tools/objectinspector/outboundconnectionsmodel.h>
#include <core/util.h>

#include <common/objectid.h>

#include <QMutexLocker>
#include <QThread>

#include <algorithm>

using namespace GammaRay;

namespace {
ObjectVisualizerModel *s_model = nullptr;
} // namespace

ObjectVisualizerModel::ObjectVisualizerModel(Probe *probe, QObject *parent)
    : QAbstractTableModel(parent), m_probe(probe) {
    connect(probe, &Probe::objectCreated, this,
            &ObjectVisualizerModel::onObjectAdded);
    connect(probe, &Probe::objectDestroyed, this,
            &ObjectVisualizerModel::onObjectRemoved);

    s_model = this;
}

ObjectVisualizerModel::~ObjectVisualizerModel() {
    s_model = nullptr;
    qDeleteAll(m_edges);
}

int ObjectVisualizerModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid())
        return 0;
    QMutexLocker lock(m_probe->objectLock());
    return m_edges.count();
}

int ObjectVisualizerModel::columnCount(const QModelIndex & /*parent*/) const {
    return 4;
}

QVariant ObjectVisualizerModel::data(const QModelIndex &index, int role) const {
    if (role == Qt::DisplayRole) {
        QMutexLocker lock(m_probe->objectLock());
        auto edge = m_edges.at(index.row());
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
        auto edge = m_edges.at(index.row());
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
        auto edge = m_edges.at(index.row());
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

QVariant ObjectVisualizerModel::headerData(int section,
                                           Qt::Orientation orientation,
                                           int role) const {

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
        return "Wheight";
    default:
        return {};
    }
}

QMap<int, QVariant>
ObjectVisualizerModel::itemData(const QModelIndex &index) const {
    QMap<int, QVariant> d = QAbstractItemModel::itemData(index);
    d.insert(ObjectIdRole, data(index, ObjectIdRole));
    d.insert(ThreadIdRole, data(index, ThreadIdRole));
    return d;
}

void ObjectVisualizerModel::onObjectAdded(QObject *object) {
    Q_ASSERT(thread() == QThread::currentThread());
    Q_ASSERT(!m_senderMap.contains(object));
    QObject *sender = object;
    auto connections =
        OutboundConnectionsModel::outboundConnectionsForObject(sender);
    for (const auto &connection : connections) {
        QObject *receiver = connection.endpoint.data();
        const bool update = m_senderMap.contains(sender) &&
            m_senderMap[sender].contains(receiver);
        Edge *edge = m_senderMap[sender][receiver];
        if (!update) {
            edge = new Edge;
            const int row = m_edges.count();
            beginInsertRows(QModelIndex(), row, row);
            m_edges.append(edge);
            m_senderMap[sender][receiver] = edge;
        }
        m_senderMap[sender][receiver]->sender = sender;
        m_senderMap[sender][receiver]->receiver = receiver;
        m_senderMap[sender][receiver]->value++;
        if (update) {
            const int row = m_edges.indexOf(edge);
            emit dataChanged(index(row, CountColumn), index(row, CountColumn));
        } else
            endInsertRows();
    }
}

void ObjectVisualizerModel::onObjectRemoved(QObject *object) {
    Q_ASSERT(thread() == QThread::currentThread());
    if (!m_senderMap.contains(object))
        return;

    for (auto edge : m_senderMap.value(object).values()) {
        const auto row = m_edges.indexOf(edge);
        beginRemoveRows(QModelIndex(), row, row);
        m_edges.removeAt(row);
        delete edge;
        endRemoveRows();
    }
    m_senderMap.remove(object);
}
