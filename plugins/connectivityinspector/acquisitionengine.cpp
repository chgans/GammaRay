#include "acquisitionengine.h"

#include "connectionmodel.h"
#include "connectiontypemodel.h"
#include "connectivityinspectorcommon.h"
#include "discriminatorproxymodel.h"

#include <core/objecttreemodel.h>
#include <core/objecttypefilterproxymodel.h>
#include <core/probe.h>
#include <core/remote/serverproxymodel.h>
#include <core/singlecolumnobjectproxymodel.h>
#include <core/tools/metaobjectbrowser/metaobjecttreemodel.h>

#include <common/objectbroker.h>

#include <QElapsedTimer>
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
    m_objectFilterModel->increaseUsageCount(object);
    m_classFilterModel->increaseUsageCount(object->metaObject());
    m_threadFilterModel->increaseUsageCount(object->thread());
    auto connections = OutboundConnectionsModel::outboundConnectionsForObject(object);
    for (const auto &connection : connections) {
        m_connectionFilterModel->increaseUsageCount(connection.type);
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
    m_objectFilterModel->decreaseUsageCount(object);
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

bool AcquisitionEngine::isValidObject(QObject *object)
{
    if (!m_probe->isValidObject(object) || m_probe->filterObject(object)) {
        return false;
    }
    auto senderThread = object->thread();
    if (!m_probe->isValidObject(senderThread) || m_probe->filterObject(senderThread)) {
        m_connectionModel->removeSender(senderThread);
        return false;
    }
    return true;
}

void AcquisitionEngine::sampleObject(QObject *sender)
{
    auto senderThread = sender->thread();
    const bool isRecordingObject = m_threadFilterModel->isAccepting(sender->thread())
                                   && m_classFilterModel->isAccepting(sender->metaObject())
                                   && m_objectFilterModel->isAccepting(sender);
    if (m_connectionModel->hasSender(sender) && !isRecordingObject) {
        m_connectionModel->removeSender(sender);
        return;
    }
    const QString senderLabel = Util::shortDisplayString(sender);

    auto connections = OutboundConnectionsModel::outboundConnectionsForObject(sender);
    for (const auto &connection : connections) {
        QObject *receiver = connection.endpoint.data();
        const bool isRecordingConnection = m_connectionFilterModel->isAccepting(connection.type);
        if (isRecordingConnection) {
            if (!isValidObject(receiver)) {
                // FIXME: remove receiver?
                continue;
            }
            auto receiverThread = receiver->thread();
            const QString receiverLabel = Util::shortDisplayString(receiver);

            // FIXME: need to track signal/slot indexes
            m_connectionModel->addConnection(sender,
                                             senderThread,
                                             senderLabel,
                                             receiver,
                                             receiverThread,
                                             receiverLabel);
        } else
            m_connectionModel->removeConnection(sender, receiver);
    }
}

void AcquisitionEngine::takeSample()
{
    DEBUG;
    QElapsedTimer timer;
    timer.start();
    QMutexLocker locker(m_probe->objectLock());
    for (const auto sender : m_objectFilterModel->targets()) {
        if (!isValidObject(sender)) {
            m_connectionModel->removeSender(sender);
            continue;
        }
        sampleObject(sender);
    }
    emit samplingDone(timer.elapsed());
}

void AcquisitionEngine::clearSamples() {
    QMutexLocker locker(m_probe->objectLock());
    m_connectionModel->clear();
}

/*
 * Dimensional filter models
 */
void AcquisitionEngine::registerConnectionModel() {
    m_connectionModel = new ConnectionModel(this);

    // TBD: sort/filter on gui side?
    auto proxy = new ServerProxyModel<QSortFilterProxyModel>(this);
    proxy->setSourceModel(m_connectionModel);

    m_probe->registerModel(ConnectivityInspector::ConnectionModelId, m_connectionModel);
}

void AcquisitionEngine::registerConnectionFilterModel() {
    m_connectionInputModel = new ConnectionTypeModel(this);
    m_connectionFilterModel = new DiscriminatorProxyModel<int, ConnectionTypeModel::TypeRole>(this);

    // TBD: sort/filter on gui side?
    auto proxy = new ServerProxyModel<QSortFilterProxyModel>(this);
    proxy->setSourceModel(m_connectionFilterModel);

    m_probe->registerModel(ConnectivityInspector::ConnectionTypeModelId, proxy);
    new DiscriminatorInterfaceBridge("Connection", m_connectionFilterModel, this);
}

void AcquisitionEngine::registerThreadFilterModel() {
    auto input = new ObjectTypeFilterProxyModel<QThread>(this);
    input->setSourceModel(m_probe->objectListModel());
    m_threadInputModel = input;
    m_threadFilterModel = new DiscriminatorProxyModel<QObject *, ObjectModel::ObjectRole>(this);

    // TBD: sort/filter on gui side?
    auto proxy = new ServerProxyModel<QSortFilterProxyModel>(this);
    proxy->setSourceModel(m_threadFilterModel);

    m_probe->registerModel(ConnectivityInspector::ThreadModelId, proxy);
    new DiscriminatorInterfaceBridge("Thread", m_threadFilterModel, this);
}

void AcquisitionEngine::registerClassFilterModel() {
    auto input = new SingleColumnObjectProxyModel();
    input->setSourceModel(m_probe->metaObjectTreeModel());
    m_classInputModel = input;
    m_classFilterModel
        = new DiscriminatorProxyModel<const QMetaObject *, QMetaObjectModel::MetaObjectRole>(this);

    // TBD: sort/filter on gui side?
    auto proxy = new ServerProxyModel<QSortFilterProxyModel>(this);
    proxy->setSourceModel(m_classFilterModel);

    m_probe->registerModel(ConnectivityInspector::ClassModelId, proxy);
    new DiscriminatorInterfaceBridge("Class", m_classFilterModel, this);
}

void AcquisitionEngine::registerObjectFilterModel() {
    m_objectInputModel = m_probe->objectTreeModel();
    m_objectFilterModel = new DiscriminatorProxyModel<QObject *, ObjectModel::ObjectRole>(this);

    // TBD: sort/filter on gui side?
    auto proxy = new ServerProxyModel<QSortFilterProxyModel>(this);
    proxy->setSourceModel(m_objectFilterModel);

    m_probe->registerModel(ConnectivityInspector::ObjectModelId, proxy);
    new DiscriminatorInterfaceBridge("Object", m_objectFilterModel, this);
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
