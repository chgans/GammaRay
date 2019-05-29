/*
  This file is part of GammaRay, the Qt application inspection and
  manipulation tool.

  Copyright (C) 2010-2019 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Kevin Funk <kevin.funk@kdab.com>

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

#include "objectvisualizer.h"
#include "connectiontypemodel.h"
#include "objectvisualizercommon.h"
#include "objectvisualizermodel.h"
#include "recordingproxymodel.h"

#include <core/objecttreemodel.h>
#include <core/objecttypefilterproxymodel.h>
#include <core/remote/serverproxymodel.h>
#include <core/singlecolumnobjectproxymodel.h>
#include <core/tools/metaobjectbrowser/metaobjecttreemodel.h>

#include <common/objectbroker.h>

#include <QMutexLocker>
#include <QQueue>
#include <QThread>
#include <QTimer>

#include <QtPlugin>

Q_DECLARE_METATYPE(QObject *)
Q_DECLARE_METATYPE(QThread *)
Q_DECLARE_METATYPE(const QMetaObject *)

using namespace GammaRay;

namespace {
constexpr int s_initialisationDelayMs = 0;
constexpr int s_updateIntervalMs = 100;
//std::chrono::milliseconds(1000);
} // namespace

ConnectivityAnalyser::ConnectivityAnalyser(Probe *probe, QObject *parent)
    : ObjectVisualizerInterface(parent)
    , m_probe(probe)
    , m_updateTimer(new QTimer(this))
{
    //m_probe->allQObjects(); // FIXME

    registerConnectionTypeModel();
    registerConnectionModel();
    registerThreadModel();
    registerClassModel();
    registerObjectModel();

    QTimer::singleShot(s_initialisationDelayMs, this, &ConnectivityAnalyser::initialise);
    m_updateTimer->setInterval(s_updateIntervalMs);
    connect(m_updateTimer, &QTimer::timeout, this, &ConnectivityAnalyser::processPendingChanges);
}

ConnectivityAnalyser::~ConnectivityAnalyser() = default;

void ConnectivityAnalyser::initialise()
{
    QMutexLocker locker(m_probe->objectLock());
    for (auto object : m_probe->allQObjects()) {
        addObject(object);
    }
    //    connect(m_probe, &Probe::objectCreated, this, &ConnectivityAnalyser::addObject);
    //    connect(m_probe, &Probe::objectDestroyed, this, &ConnectivityAnalyser::removeObject);
    //    connect(m_connectionTypeModel,
    //            &ConnectionTypeModel::modelReset,
    //            this,
    //            &ConnectivityAnalyser::refine);
    //    connect(m_connectionTypeModel,
    //            &ConnectionTypeModel::recordingChanged,
    //            this,
    //            &ConnectivityAnalyser::refine);
    //    m_updateTimer->start();
}

void ConnectivityAnalyser::refine()
{
    QMutexLocker locker(m_probe->objectLock());
    m_connectionModel->clear();
    for (auto object : m_probe->allQObjects()) {
        addObject(object);
    }
}

void ConnectivityAnalyser::registerConnectionTypeModel()
{
    m_connectionTypeModel = new ConnectionTypeModel(this);
    auto connectionTypeProxy = new ServerProxyModel<QSortFilterProxyModel>(this);
    connectionTypeProxy->setSourceModel(m_connectionTypeModel);
    m_probe->registerModel(ObjectVisualizerConnectionTypeModelId, connectionTypeProxy);
}

void ConnectivityAnalyser::registerConnectionModel()
{
    m_connectionModel = new ConnectionModel(m_probe, this);
    auto proxy = new ServerProxyModel<QSortFilterProxyModel>(this);
    proxy->setSourceModel(m_connectionModel);
    m_probe->registerModel(ObjectVisualizerConnectionModelId, proxy);
}

void ConnectivityAnalyser::registerThreadModel()
{
    auto proxy = new ObjectTypeFilterProxyModel<QThread>(this);
    proxy->setSourceModel(m_probe->objectListModel());
    m_threadModel = new RecordingProxyModel<QObject *, ObjectModel::ObjectRole>(this);
    m_threadModel->setSourceModel(proxy);
    m_probe->registerModel(ObjectVisualizerThreadModelId, m_threadModel);
}

void ConnectivityAnalyser::registerClassModel()
{
    auto model = new SingleColumnObjectProxyModel();
    model->setSourceModel(m_probe->metaObjectTreeModel());
    m_classModel = new RecordingProxyModel<const QMetaObject *, QMetaObjectModel::MetaObjectRole>(
        this);
    m_classModel->setSourceModel(model);
    m_probe->registerModel(ObjectVisualizerClassModelId, m_classModel);
}

void ConnectivityAnalyser::registerObjectModel()
{
    m_objectModel = new RecordingProxyModel<QObject *, ObjectModel::ObjectRole>(this);
    m_objectModel->setSourceModel(m_probe->objectTreeModel());
    m_probe->registerModel(ObjectVisualizerObjectModelId, m_objectModel);
}

void ConnectivityAnalyser::addObject(QObject *object)
{
    if (!m_probe->isValidObject(object))
        return;
    auto connections = OutboundConnectionsModel::outboundConnectionsForObject(object);
    for (const auto &connection : connections) {
        QObject *receiver = connection.endpoint.data();
        const auto type = static_cast<Qt::ConnectionType>(connection.type);
        if (!m_connectionTypeModel->isRecording(type))
            continue;
        m_connectionModel->addOutboundConnection(object, receiver);
        m_connectionTypeModel->increaseCount(connection.type);
    }
    m_classModel->increaseCount(object->metaObject());
    m_objectModel->increaseCount(object);
    if (object->inherits("QThread")) {
        m_threadModel->increaseCount(object);
    }
}

void ConnectivityAnalyser::removeObject(QObject *object)
{
    m_connectionModel->removeOutboundConnections(object);
    // TODO?
    // connections = m_connectionModel->removeConnections(object);
    //m_connectionTypeModel->decreaseCount(connections);
}

void ConnectivityAnalyser::processPendingChanges()
{
    QMutexLocker locker(m_probe->objectLock());
    while (!m_objectAdded.isEmpty())
        addObject(m_objectAdded.dequeue());
    while (!m_objectRemoved.isEmpty())
        removeObject(m_objectRemoved.dequeue());
}

void ConnectivityAnalyser::scheduleAddObject(QObject *object)
{
    m_objectAdded.removeAll(object);
    m_objectRemoved.removeAll(object);
    m_objectAdded.enqueue(object);
}

void ConnectivityAnalyser::scheduleRemoveObject(QObject *object)
{
    m_objectAdded.removeAll(object);
    m_objectRemoved.removeAll(object);
    m_objectRemoved.enqueue(object);
}

void ConnectivityAnalyser::clearHistory()
{
    m_connectionModel->clear();
    m_connectionTypeModel->resetCounts();
    m_objectAdded.clear();
    m_objectRemoved.clear();
}

void ConnectivityAnalyser::recordAll()
{
    m_connectionTypeModel->recordAll();
}

void ConnectivityAnalyser::recordNone()
{
    m_connectionTypeModel->recordNone();
}

void ConnectivityAnalyser::showAll()
{
    m_connectionTypeModel->showAll();
}

void ConnectivityAnalyser::showNone()
{
    m_connectionTypeModel->showNone();
}
