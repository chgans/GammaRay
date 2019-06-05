#include "acquisitionengine.h"

#include "connectionmodel.h"
#include "connectiontypemodel.h"
#include "connectivityinspectorcommon.h"
#include "filterproxymodel.h"

#include <core/objecttreemodel.h>
#include <core/objecttypefilterproxymodel.h>
#include <core/probe.h>
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

#define DEBUG qDebug() << __FUNCTION__ << __LINE__

using namespace GammaRay;

AcquisitionEngine::AcquisitionEngine(Probe *probe, QObject *parent)
    : AcquisitionInterface(parent), m_timer(new QTimer(this)), m_probe(probe) {
    setBufferSize(100);
    setSamplingRate(5);

    registerClassFilterModel();
    registerConnectionFilterModel();
    registerObjectFilterModel();
    registerThreadFilterModel();

    registerConnectionModel();

    m_threadFilterModel->setSourceModel(m_threadInputModel);
    m_connectionFilterModel->setSourceModel(m_connectionInputModel);
    m_classFilterModel->setSourceModel(m_classInputModel);
    m_objectFilterModel->setSourceModel(m_objectInputModel);

    connect(m_timer, &QTimer::timeout, this, &AcquisitionEngine::takeSample);

    connect(m_probe, &Probe::objectCreated, this,
            &AcquisitionEngine::increaseCountersForObject);
    connect(m_probe, &Probe::objectDestroyed, this,
            &AcquisitionEngine::decreaseCountersForObject);
}

AcquisitionEngine::~AcquisitionEngine() = default;

void AcquisitionEngine::increaseCountersForObject(QObject *object) {
    if (!m_probe->isValidObject(object))
        return;
    if (!m_probe->isValidObject(object->thread()))
        return;
    if (m_probe->filterObject(object))
        return;
    m_objectFilterModel->increaseCount(object);
    m_classFilterModel->increaseCount(object->metaObject());
    if (m_probe->isValidObject(object->thread()))
        m_threadFilterModel->increaseCount(object->thread());
    auto connections = OutboundConnectionsModel::outboundConnectionsForObject(object);
    for (const auto &connection : connections) {
        m_connectionFilterModel->increaseCount(connection.type);
    }
}

void AcquisitionEngine::decreaseCountersForObject(QObject *object) {
    if (!m_probe->isValidObject(object))
        return;
    if (!m_probe->isValidObject(object->thread()))
        return;
    if (m_probe->filterObject(object))
        return;

    // FIXME:
    m_objectFilterModel->decreaseCount(object);
    // m_threadFilterModel->decreaseCount(object->thread());
    // m_classFilterModel->increaseCount(object->metaObject());
    // m_connectionTypeModel->decreaseCount(connections);
}

void AcquisitionEngine::startSampling() {
    DEBUG;
    //    m_threadFilterModel->setSourceModel(m_threadInputModel);
    //    m_connectionFilterModel->setSourceModel(m_connectionInputModel);
    //    m_classFilterModel->setSourceModel(m_classInputModel);
    //    m_objectFilterModel->setSourceModel(m_objectInputModel);
}

void AcquisitionEngine::stopSampling() {
    DEBUG;
    //    m_threadFilterModel->setSourceModel(nullptr);
    //    m_connectionFilterModel->setSourceModel(nullptr);
    //    m_classFilterModel->setSourceModel(nullptr);
    //    m_objectFilterModel->setSourceModel(nullptr);
}

void AcquisitionEngine::pauseSampling() { DEBUG; }

void AcquisitionEngine::resumeSampling() { DEBUG; }

void AcquisitionEngine::takeSample() {
    DEBUG;
    QMutexLocker locker(m_probe->objectLock());
    for (const auto object : m_objectFilterModel->targets()) {
        if (!m_probe->isValidObject(object)) {
            m_connectionModel->removeConnections(object);
            continue;
        }
        const bool isRecordingObject =
            m_threadFilterModel->isRecording(object->thread()) ||
            m_classFilterModel->isRecording(object->metaObject()) ||
            m_objectFilterModel->isRecording(object);
        auto connections =
            OutboundConnectionsModel::outboundConnectionsForObject(object);
        for (const auto &connection : connections) {
            QObject *receiver = connection.endpoint.data();
            const bool isRecordingConnection =
                m_connectionFilterModel->isRecording(connection.type);
            const bool isRecorded =
                m_connectionModel->hasConnection(object, receiver);
            const bool isRecording = isRecordingObject || isRecordingConnection;
            if (isRecorded == isRecording)
                continue;
            if (isRecording)
                m_connectionModel->addConnection(object, receiver);
            else
                m_connectionModel->removeConnection(object, receiver);
        }
    }
}

void AcquisitionEngine::clearSamples() {
    QMutexLocker locker(m_probe->objectLock());
    m_connectionModel->clear();
}

/*
 * Dimensional filter models
 */
void AcquisitionEngine::registerConnectionModel() {
    m_connectionModel = new ConnectionModel(m_probe, this);
    auto proxy = new ServerProxyModel<QSortFilterProxyModel>(this);
    proxy->setSourceModel(m_connectionModel);
    m_probe->registerModel(ObjectVisualizerConnectionModelId, proxy);
}

void AcquisitionEngine::registerConnectionFilterModel() {
    m_connectionInputModel = new ConnectionTypeModel(this);
    m_connectionFilterModel =
        new FilterProxyModel<int, ConnectionTypeModel::TypeRole>(this);

    auto proxy = new ServerProxyModel<QSortFilterProxyModel>(this);
    proxy->setSourceModel(m_connectionFilterModel);

    m_probe->registerModel(ObjectVisualizerConnectionTypeModelId, proxy);
    new FilterInterfaceBridge("Connection", m_connectionFilterModel, this);
}

void AcquisitionEngine::registerThreadFilterModel() {
    auto input = new ObjectTypeFilterProxyModel<QThread>(this);
    input->setSourceModel(m_probe->objectListModel());
    m_threadInputModel = input;
    m_threadFilterModel =
        new FilterProxyModel<QObject *, ObjectModel::ObjectRole>(this);

    auto proxy = new ServerProxyModel<QSortFilterProxyModel>(this);
    proxy->setSourceModel(m_threadFilterModel);

    m_probe->registerModel(ObjectVisualizerThreadModelId, m_threadFilterModel);
    new FilterInterfaceBridge("Thread", m_threadFilterModel, this);
}

void AcquisitionEngine::registerClassFilterModel() {
    auto input = new SingleColumnObjectProxyModel();
    input->setSourceModel(m_probe->metaObjectTreeModel());
    m_classInputModel = input;
    m_classFilterModel =
        new FilterProxyModel<const QMetaObject *,
                             QMetaObjectModel::MetaObjectRole>(this);

    auto proxy = new ServerProxyModel<QSortFilterProxyModel>(this);
    proxy->setSourceModel(m_classFilterModel);

    m_probe->registerModel(ObjectVisualizerClassModelId, m_classFilterModel);
    new FilterInterfaceBridge("Class", m_classFilterModel, this);
}

void AcquisitionEngine::registerObjectFilterModel() {
    m_objectInputModel = m_probe->objectTreeModel();
    m_objectFilterModel =
        new FilterProxyModel<QObject *, ObjectModel::ObjectRole>(this);

    auto proxy = new ServerProxyModel<QSortFilterProxyModel>(this);
    proxy->setSourceModel(m_objectFilterModel);

    m_probe->registerModel(ObjectVisualizerObjectModelId, m_objectFilterModel);
    new FilterInterfaceBridge("Object", m_objectFilterModel, this);
}

/*
 * High level logic
 */

AcquisitionInterface::State AcquisitionEngine::state() const {
    DEBUG;
    if (m_timer->isActive())
        return Started;
    if (m_isPaused)
        return Paused;
    return Stopped;
}

int AcquisitionEngine::bufferSize() const {
    DEBUG << m_bufferSize;
    return m_bufferSize;
}

qreal AcquisitionEngine::bufferUsage() const {
    DEBUG;
    return m_connectionQueue.count() * qreal(100) / m_bufferSize;
}

int AcquisitionEngine::bufferOverrunCount() const {
    DEBUG;
    return m_bufferOverrunCount;
}

qreal AcquisitionEngine::samplingRate() const
{
    DEBUG;
    return m_samplingRate;
}

void AcquisitionEngine::start() {
    DEBUG;
    if (state() != Stopped)
        return;
    m_timer->start(samplingPeriodMs());
    emit stateChanged(state());
    startSampling();
}

void AcquisitionEngine::stop() {
    DEBUG;
    if (state() == Stopped)
        return;

    m_isPaused = false;
    m_timer->stop();
    emit stateChanged(state());
    stopSampling();
}

void AcquisitionEngine::pause() {
    DEBUG;
    if (state() != Started)
        return;
    m_isPaused = true;
    emit stateChanged(state());
    pauseSampling();
}

void AcquisitionEngine::resume() {
    DEBUG;
    if (state() != Paused)
        return;
    m_isPaused = false;
    emit stateChanged(state());
    resumeSampling();
}

void AcquisitionEngine::refresh() {
    DEBUG;
    takeSample();
}

void AcquisitionEngine::clear() {
    DEBUG;
    m_bufferOverrunCount = 0;
    clearSamples();
}

void AcquisitionEngine::setBufferSize(int size) {
    DEBUG;
    if (m_bufferSize == size)
        return;
    if (size < 1)
        return;
    m_bufferSize = size;
    // TODO: MiB to count
    emit bufferSizeChanged(m_bufferSize);
}

void AcquisitionEngine::setSamplingRate(qreal rate)
{
    DEBUG << "requested" << rate;
    if (qFuzzyCompare(m_samplingRate, rate))
        return;
    if (rate <= 0. || rate >= 100.)
        return;
    m_samplingRate = rate;
    DEBUG << "period" << samplingPeriodMs();
    m_timer->setInterval(samplingPeriodMs());
    DEBUG << "actual" << m_samplingRate;
    emit samplingRateChanged(m_samplingRate);
}

int AcquisitionEngine::samplingPeriodMs() const {
    return qRound(1. / static_cast<qreal>(m_samplingRate) * 1000);
}
