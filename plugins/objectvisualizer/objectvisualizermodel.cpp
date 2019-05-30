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
#include <core/tools/objectinspector/outboundconnectionsmodel.h>
#include <core/util.h>

#include <QMutexLocker>
#include <QThread>

#include <algorithm>

#include <iostream>

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
    qDeleteAll(m_counterList);
}

int ObjectVisualizerModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid())
        return 0;
    QMutexLocker lock(m_probe->objectLock());
    return m_counterList.count();
}

int ObjectVisualizerModel::columnCount(const QModelIndex & /*parent*/) const {
    return 4;
}

QVariant ObjectVisualizerModel::data(const QModelIndex &index, int role) const {
    if (role != Qt::DisplayRole)
        return {};

    QMutexLocker lock(m_probe->objectLock());
    auto counter = m_counterList.at(index.row());
    switch (index.column()) {
    case ThreadColumn:
        if (m_probe->isValidObject(counter->thread))
            return Util::shortDisplayString(counter->thread);
        return Util::addressToString(counter->thread);
    case SenderColumn:
        if (m_probe->isValidObject(counter->sender))
            return Util::shortDisplayString(counter->sender);
        return Util::addressToString(counter->sender);
    case ReceiverColumn:
        if (m_probe->isValidObject(counter->receiver))
            return Util::shortDisplayString(counter->receiver);
        return Util::addressToString(counter->receiver);
    case CountColumn:
        return counter->value;
    default:
        return {};
    }
}

QVariant ObjectVisualizerModel::headerData(int section,
                                           Qt::Orientation orientation,
                                           int role) const {

    if (role != Qt::DisplayRole)
        return {};
    if (orientation != Qt::Horizontal)
        return {};
    switch (section) {
    case ThreadColumn:
        return "Thread";
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

void ObjectVisualizerModel::onObjectAdded(QObject *sender) {
    Q_ASSERT(thread() == QThread::currentThread());

    std::cout << __FUNCTION__ << " " << sender << " "
              << sender->objectName().toUtf8().constData() << std::endl;
    const auto connections =
        OutboundConnectionsModel::outboundConnectionsForObject(sender);
    if (connections.isEmpty())
        return;
    auto thread = sender->thread();
    m_senderThreads.insert(sender, thread);
    SenderTree &senderTree = m_threadTree[thread];
    assert(!senderTree.contains(sender));
    ReceiverMap &receiverMap = senderTree[sender];
    for (const auto &connection : connections) {
        auto receiver = connection.endpoint.data();
        if (receiverMap.contains(receiver)) {
            Counter *counter = receiverMap.value(receiver);
            assert(counter->thread == thread);
            assert(counter->sender == sender);
            assert(counter->receiver == receiver);
            counter->value++;
        } else {
            const int row = m_counterList.count();
            beginInsertRows(QModelIndex(), row, row);
            auto counter = new Counter{thread, sender, receiver, 1};
            m_counterList.append(counter);
            receiverMap.insert(receiver, counter);
            endInsertRows();
        }
    }
}

void ObjectVisualizerModel::onObjectRemoved(QObject *object) {
    {
        std::cout << __FUNCTION__ << " " << object << std::endl;
        QObject *sender = object;
        if (!m_senderThreads.contains(sender))
            return;
        QThread *thread = m_senderThreads.value(sender);
        assert(m_threadTree.contains(thread));
        const ReceiverMap &receiverMap = m_threadTree[thread][sender];
        assert(!receiverMap.isEmpty());
        for (QObject *receiver : receiverMap.keys()) {
            Counter *counter = receiverMap.value(receiver);
            const int row = m_counterList.indexOf(counter);
            beginRemoveRows(QModelIndex(), row, row);
            m_counterList.removeAt(row);
            delete counter;
            endRemoveRows();
        }
        m_threadTree[thread].remove(sender);
        if (m_threadTree[thread].isEmpty()) {
            m_threadTree.remove(thread);
            m_senderThreads.remove(sender);
        }
    }
    // TODO: update connection counter of receivers that were connected to
    // this sender
}
