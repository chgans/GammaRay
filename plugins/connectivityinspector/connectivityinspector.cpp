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

#include "connectivityinspector.h"
#include "connectiontypemodel.h"
#include "connectivityinspectorcommon.h"
#include "connectivityinspectormodel.h"
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
constexpr int s_updateIntervalMs = 1000;
} // namespace

ConnectivityAnalyser::ConnectivityAnalyser(Probe *probe, QObject *parent)
    : ConnectivityInspectorInterface(parent), m_probe(probe),
      m_updateTimer(new QTimer(this)) {

    registerConnectionModel();

    registerConnectionRecordingModel();
    registerThreadRecordingModel();
    registerClassRecordingModel();
    registerObjectRecodingModel();

    QTimer::singleShot(s_initialisationDelayMs, this, &ConnectivityAnalyser::initialise);
    m_updateTimer->setInterval(s_updateIntervalMs);
    connect(m_updateTimer, &QTimer::timeout, this,
            &ConnectivityAnalyser::update);
}

ConnectivityAnalyser::~ConnectivityAnalyser() = default;

void ConnectivityAnalyser::initialise() {
    QMutexLocker locker(m_probe->objectLock());
    for (auto object : m_probe->allQObjects()) {
        addObject(object);
    }

    connect(m_probe, &Probe::objectCreated, this,
            &ConnectivityAnalyser::addObject);

    connect(m_probe, &Probe::objectDestroyed, this,
            &ConnectivityAnalyser::removeObject);

    //    connect(m_connectionTypeModel,
    //            &ConnectionTypeModel::modelReset,
    //            this,
    //            &ConnectivityAnalyser::refine);
    //    connect(m_connectionTypeModel,
    //            &ConnectionTypeModel::recordingChanged,
    //            this,
    //            &ConnectivityAnalyser::refine);

    m_updateTimer->start();
}

void ConnectivityAnalyser::update() {

    if (isPaused()) // FIXME
        return;

    QMutexLocker locker(m_probe->objectLock());
    for (const auto object : m_objectRecordingModel->targets()) {
        if (!m_probe->isValidObject(object)) {
            m_connectionModel->removeConnections(object);
            continue;
        }
        const bool isRecordingObject =
            m_threadRecordingModel->isRecording(object->thread()) ||
            m_classRecordingModel->isRecording(object->metaObject()) ||
            m_objectRecordingModel->isRecording(object);
        auto connections =
            OutboundConnectionsModel::outboundConnectionsForObject(object);
        for (const auto &connection : connections) {
            QObject *receiver = connection.endpoint.data();
            const bool isRecodingConnection =
                m_connectionRecordingModel->isRecording(connection.type);
            const bool isRecorded =
                m_connectionModel->hasConnection(object, receiver);
            const bool isRecording = isRecordingObject || isRecodingConnection;
            if (isRecorded == isRecording)
                continue;
            if (isRecording)
                m_connectionModel->addConnection(object, receiver);
            else
                m_connectionModel->removeConnection(object, receiver);
        }
    }
}

void ConnectivityAnalyser::clearHistory() {
    QMutexLocker locker(m_probe->objectLock());
    m_connectionModel->clear();
    m_connectionRecordingModel->resetCounts();
    m_objectAdded.clear();
    m_objectRemoved.clear();
}

void ConnectivityAnalyser::addObject(QObject *object) {
    if (!m_probe->isValidObject(object))
        return;
    if (m_probe->filterObject(object))
        return;
    m_objectRecordingModel->increaseCount(object);
    m_classRecordingModel->increaseCount(object->metaObject());
    if (m_probe->isValidObject(object->thread()))
        m_threadRecordingModel->increaseCount(object->thread());
    auto connections = OutboundConnectionsModel::outboundConnectionsForObject(object);
    for (const auto &connection : connections) {
        QObject *receiver = connection.endpoint.data();
        m_connectionRecordingModel->increaseCount(connection.type);
        if (!m_connectionRecordingModel->isRecording(connection.type))
            continue;
        if (!m_threadRecordingModel->isRecording(object->thread()))
            continue;
        if (m_classRecordingModel->isRecording(object->metaObject()))
            continue;
        if (!m_objectRecordingModel->isRecording(object))
            return;
        if (isPaused()) {

        } else {
            m_connectionModel->addConnection(object, receiver);
        }
    }
}

void ConnectivityAnalyser::removeObject(QObject *object)
{
    // qDebug() << __FUNCTION__ << Util::addressToString(object);
    m_connectionModel->removeConnections(object);
    // TODO?
    // connections = m_connectionModel->removeConnections(object);
    // m_connectionTypeModel->decreaseCount(connections);
}

void ConnectivityAnalyser::registerConnectionModel() {
    m_connectionModel = new ConnectionModel(m_probe, this);
    auto proxy = new ServerProxyModel<QSortFilterProxyModel>(this);
    proxy->setSourceModel(m_connectionModel);
    m_probe->registerModel(ObjectVisualizerConnectionModelId, proxy);
}

void ConnectivityAnalyser::registerConnectionRecordingModel() {
    auto model = new ConnectionTypeModel(this);
    m_connectionRecordingModel =
        new RecordingProxyModel<int, ConnectionTypeModel::TypeRole>(this);
    m_connectionRecordingModel->setSourceModel(model);
    auto connectionTypeProxy =
        new ServerProxyModel<QSortFilterProxyModel>(this);
    connectionTypeProxy->setSourceModel(m_connectionRecordingModel);
    m_probe->registerModel(ObjectVisualizerConnectionTypeModelId,
                           connectionTypeProxy);
    new RecordingInterfaceBridge("Connection", m_connectionRecordingModel,
                                 this);
}

void ConnectivityAnalyser::registerThreadRecordingModel() {
    auto proxy = new ObjectTypeFilterProxyModel<QThread>(this);
    proxy->setSourceModel(m_probe->objectListModel());
    m_threadRecordingModel =
        new RecordingProxyModel<QObject *, ObjectModel::ObjectRole>(this);
    m_threadRecordingModel->setSourceModel(proxy);
    //    auto threadProxy = new ServerProxyModel<QSortFilterProxyModel>(this);
    //    threadProxy->setSourceModel(m_threadRecordingModel);
    m_probe->registerModel(ObjectVisualizerThreadModelId,
                           m_threadRecordingModel);
    new RecordingInterfaceBridge("Thread", m_threadRecordingModel, this);
}

void ConnectivityAnalyser::registerClassRecordingModel() {
    auto model = new SingleColumnObjectProxyModel();
    model->setSourceModel(m_probe->metaObjectTreeModel());
    m_classRecordingModel =
        new RecordingProxyModel<const QMetaObject *,
                                QMetaObjectModel::MetaObjectRole>(this);
    m_classRecordingModel->setSourceModel(model);
    m_probe->registerModel(ObjectVisualizerClassModelId, m_classRecordingModel);
    new RecordingInterfaceBridge("Class", m_classRecordingModel, this);
}

void ConnectivityAnalyser::registerObjectRecodingModel() {
    m_objectRecordingModel =
        new RecordingProxyModel<QObject *, ObjectModel::ObjectRole>(this);
    m_objectRecordingModel->setSourceModel(m_probe->objectTreeModel());
    m_probe->registerModel(ObjectVisualizerObjectModelId,
                           m_objectRecordingModel);
    new RecordingInterfaceBridge("Object", m_objectRecordingModel, this);
}
