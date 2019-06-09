#include "acquisitionengine.h"

#include "connectionmodel.h"
#include "connectiontypemodel.h"
#include "connectivityinspectorcommon.h"
#include "discriminatorproxymodel.h"

#include <3rdparty/kde/krecursivefilterproxymodel.h>
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
Q_DECLARE_METATYPE(const void *)

#define DEBUG qDebug() << __FUNCTION__ << __LINE__

namespace {
using IndexVisitor = std::function<void(const QModelIndex &)>;
void visitModelIndex(const QAbstractItemModel *model, const QModelIndex &index, IndexVisitor visitor)
{
    const int rowCount = model->rowCount(index);
    for (int row = 0; row < rowCount; ++row) {
        const auto childIndex = model->index(row, 0, index);
        visitor(childIndex);
        visitModelIndex(model, childIndex, visitor);
    }
}
} // namespace

using namespace GammaRay;
using namespace GammaRay::CI;

AcquisitionEngine::AcquisitionEngine(Probe *probe, QObject *parent)
    : AcquisitionInterface(parent), m_timer(new QTimer(this)), m_probe(probe) {
    setBufferSize(100);
    setSamplingRate(5);

    //registerTypeDiscriminator();
    registerConnectionDiscriminator();
    //registerClassDiscriminator();
    registerObjectDiscriminator();
    //registerThreadDiscriminator();

    connect(m_timer, &QTimer::timeout, this, &AcquisitionEngine::takeSample);

    // TODO: Move the counters out of here (see end of file)
#if 0
    connect(m_probe, &Probe::objectCreated, this, &AcquisitionEngine::increaseCountersForObject);
    connect(m_probe, &Probe::objectDestroyed, this, &AcquisitionEngine::decreaseCountersForObject);
#endif
}

AcquisitionEngine::~AcquisitionEngine() = default;

void AcquisitionEngine::startSampling()
{
    DEBUG;
}

void AcquisitionEngine::stopSampling()
{
    DEBUG;
}

void AcquisitionEngine::pauseSampling()
{
    DEBUG;
}

void AcquisitionEngine::resumeSampling()
{
    DEBUG;
}

bool AcquisitionEngine::isValidObject(QObject *object)
{
    if (!m_probe->isValidObject(object) || m_probe->filterObject(object)) {
        return false;
    }
    //    auto senderThread = object->thread();
    //    if (!m_probe->isValidObject(senderThread) || m_probe->filterObject(senderThread)) {
    //        m_connectionModel->removeSender(senderThread);
    //        return false;
    //    }
    return true;
}

void AcquisitionEngine::sampleObject(QObject *sender)
{
    //    auto senderThread = sender->thread();
    //    const bool isRecordingObject
    //        = m_threadFilterModel->isAccepting(sender->thread())
    //          && m_classFilterModel->isAccepting(sender->metaObject())
    //          && m_objectFilterModel->isAccepting(sender);
    //    if (m_connectionModel->hasSender(sender) && !isRecordingObject) {
    //        m_connectionModel->removeSender(sender);
    //        return;
    //    }
    //    const QString senderLabel = Util::shortDisplayString(sender);

    //    auto connections = OutboundConnectionsModel::outboundConnectionsForObject(sender);
    //    for (const auto &connection : connections) {
    //        QObject *receiver = connection.endpoint.data();
    //        const bool isRecordingConnection = m_connectionFilterModel->isAccepting(connection.type);
    //        if (isRecordingConnection) {
    //            if (!isValidObject(receiver)) {
    //                // FIXME: remove receiver?
    //                continue;
    //            }
    //            auto receiverThread = receiver->thread();
    //            const QString receiverLabel = Util::shortDisplayString(receiver);

    //            // FIXME: need to track signal/slot indexes
    //            m_connectionModel->addConnection(sender,
    //                                             senderThread,
    //                                             senderLabel,
    //                                             receiver,
    //                                             receiverThread,
    //                                             receiverLabel);
    //        } else
    //            m_connectionModel->removeConnection(sender, receiver);
    //    }
}

void AcquisitionEngine::takeSample()
{
    DEBUG;
    QMutexLocker locker(m_probe->objectLock());
    QElapsedTimer timer;
    timer.start();
    const auto model = m_objectDiscriminator->filteredModel();
    visitModelIndex(model, QModelIndex(), [this](const QModelIndex &index) {
        auto sender = index.data(ObjectModel::ObjectRole).value<QObject *>();
        if (isValidObject(sender))
            sampleObject(sender);
        //        else
        //            m_connectionModel->removeSender(sender);
    });
    emit samplingDone(timer.elapsed());
}

void AcquisitionEngine::clearSamples() {
    QMutexLocker locker(m_probe->objectLock());
    m_connectionModel->clear();
}

/*
 * Dimensional filter models
 */
void AcquisitionEngine::registerTypeDiscriminator()
{
    auto typeDiscriminator = new TypeDiscriminator(typeFilterName(), this);
    typeDiscriminator->setDiscriminationRole(ConnectionTypeModel::TypeRole);
    typeDiscriminator->setSourceModel(new ConnectionTypeModel(this));
    typeDiscriminator->initialise(m_probe);
}

void AcquisitionEngine::registerConnectionDiscriminator()
{
    auto connectionDiscriminator = new ConnectionDiscriminator(connectionFilterName(), this);
    connectionDiscriminator->setDiscriminationRole(ConnectionModel::ConnectionIdRole);
    auto input = new ConnectionModel(this);
    connect(m_probe, &Probe::objectCreated, input, &ConnectionModel::addObject);
    connect(m_probe, &Probe::objectDestroyed, input, &ConnectionModel::removeObject);
    connectionDiscriminator->setSourceModel(input);
    connectionDiscriminator->initialise(m_probe);
}

void AcquisitionEngine::registerThreadDiscriminator()
{
    m_threadDiscriminator = new ThreadDiscriminator(threadFilterName(), this);
    m_threadDiscriminator->setDiscriminationRole(ObjectModel::ObjectRole);
    auto input = new ObjectTypeFilterProxyModel<QThread>(this);
    input->setSourceModel(m_probe->objectListModel());
    m_threadDiscriminator->setSourceModel(input);
    m_threadDiscriminator->initialise(m_probe);
}

void AcquisitionEngine::registerClassDiscriminator()
{
    m_classDiscriminator = new ClassDiscriminator(classFilterName(), this);
    m_classDiscriminator->setDiscriminationRole(QMetaObjectModel::MetaObjectRole);
    auto input = new SingleColumnObjectProxyModel(this);
    input->setSourceModel(m_probe->metaObjectTreeModel());
    m_classDiscriminator->setSourceModel(input);
    m_classDiscriminator->initialise(m_probe);
}

void AcquisitionEngine::registerObjectDiscriminator()
{
    m_objectDiscriminator = new ObjectDiscriminator(objectFilterName(), this);
    m_objectDiscriminator->setDiscriminationRole(QMetaObjectModel::MetaObjectRole);
    auto input = new SingleColumnObjectProxyModel(this);
    input->setSourceModel(m_probe->objectTreeModel());
    m_objectDiscriminator->setSourceModel(input);
    m_objectDiscriminator->initialise(m_probe);
}

/*
 * High level logic
 */

AcquisitionInterface::State AcquisitionEngine::state() const
{
    if (m_timer->isActive())
        return Started;
    if (m_isPaused)
        return Paused;
    return Stopped;
}

int AcquisitionEngine::bufferSize() const
{
    return m_bufferSize;
}

qreal AcquisitionEngine::bufferUsage() const
{
    return m_connectionQueue.count() * qreal(100) / m_bufferSize;
}

int AcquisitionEngine::bufferOverrunCount() const
{
    return m_bufferOverrunCount;
}

qreal AcquisitionEngine::samplingRate() const
{
    return m_samplingRate;
}

void AcquisitionEngine::start()
{
    if (state() != Stopped)
        return;
    m_timer->start(samplingPeriodMs());
    emit stateChanged(state());
    startSampling();
}

void AcquisitionEngine::stop()
{
    if (state() == Stopped)
        return;

    m_isPaused = false;
    m_timer->stop();
    emit stateChanged(state());
    stopSampling();
}

void AcquisitionEngine::pause()
{
    if (state() != Started)
        return;
    m_isPaused = true;
    emit stateChanged(state());
    pauseSampling();
}

void AcquisitionEngine::resume()
{
    if (state() != Paused)
        return;
    m_isPaused = false;
    emit stateChanged(state());
    resumeSampling();
}

void AcquisitionEngine::refresh()
{
    takeSample();
}

void AcquisitionEngine::clear()
{
    m_bufferOverrunCount = 0;
    clearSamples();
}

void AcquisitionEngine::setBufferSize(int size)
{
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
    if (qFuzzyCompare(m_samplingRate, rate))
        return;
    if (rate <= 0. || rate >= 100.)
        return;
    m_samplingRate = rate;
    m_timer->setInterval(samplingPeriodMs());
    emit samplingRateChanged(m_samplingRate);
}

int AcquisitionEngine::samplingPeriodMs() const {
    return qRound(1. / static_cast<qreal>(m_samplingRate) * 1000);
}

#if 0
void AcquisitionEngine::increaseCountersForObject(QObject *object) {
    if (!m_probe->isValidObject(object))
        return;
    if (!m_probe->isValidObject(object->thread()))
        return;
    if (m_probe->filterObject(object))
        return;
    //    m_objectFilterModel->increaseUsageCount(object);
    //    m_classFilterModel->increaseUsageCount(object->metaObject());
    //    m_threadFilterModel->increaseUsageCount(object->thread());
    //    auto connections = OutboundConnectionsModel::outboundConnectionsForObject(object);
    //    for (const auto &connection : connections) {
    //        m_connectionFilterModel->increaseUsageCount(connection.type);
    //    }
}

void AcquisitionEngine::decreaseCountersForObject(QObject *object) {
    if (!m_probe->isValidObject(object))
        return;
    if (!m_probe->isValidObject(object->thread()))
        return;
    if (m_probe->filterObject(object))
        return;

    //    // FIXME:
    //    m_objectFilterModel->decreaseUsageCount(object);
    //    // m_threadFilterModel->decreaseCount(object->thread());
    //    // m_classFilterModel->increaseCount(object->metaObject());
    //    // m_connectionTypeModel->decreaseCount(connections);
}
#endif
