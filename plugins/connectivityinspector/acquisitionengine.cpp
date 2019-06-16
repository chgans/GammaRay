#include "acquisitionengine.h"

#include "clustermodel.h"
#include "connectionmodel.h"
#include "connectiontypemodel.h"
#include "connectivityinspectorcommon.h"
#include "discriminatorproxymodel.h"
#include "edgemodel.h"
#include "vertexmodel.h"

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
using namespace GammaRay::Connectivity;

AcquisitionEngine::AcquisitionEngine(Probe *probe, QObject *parent)
    : AcquisitionInterface(parent), m_timer(new QTimer(this)), m_probe(probe) {
    setBufferSize(100);
    setSamplingRate(5);

    registerTypeDiscriminator();
    registerConnectionDiscriminator();
    registerClassDiscriminator();
    registerObjectDiscriminator();
    registerThreadDiscriminator();

    registerVertexModel();
    registerClusterModel();
    registerEdgeModel();

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

bool AcquisitionEngine::filterObject(QObject *object)
{
    if (!m_probe->isValidObject(object))
        return true;
    return m_probe->filterObject(object);
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

EdgeItem edgeItem(const QAbstractItemModel *model, int row)
{
    const auto index = model->index(row, 0);
    auto id = model->data(index, ConnectionModel::ConnectionIdRole).value<ObjectId>().id();
    auto label = model->data(index, Qt::DisplayRole).toString();
    auto sender = model->data(index, ConnectionModel::SenderObjectIdRole).value<ObjectId>().id();
    auto receiver = model->data(index, ConnectionModel::ReceiverObjectIdRole).value<ObjectId>().id();
    return {id, label, sender, receiver, 0};
}

VertexItem vertexItem(const QAbstractItemModel *model, int row)
{
    const auto index = model->index(row, 0);
    auto id = model->data(index, ObjectModel::ObjectIdRole).value<ObjectId>().id();
    auto label = model->data(index, Qt::DisplayRole).toString();
    return {id, label, 0};
}

void AcquisitionEngine::takeSample()
{
    DEBUG;
    QMutexLocker locker(m_probe->objectLock());
    QElapsedTimer timer;
    timer.start();

    QSet<QObject *> objects;
    QSet<QThread *> threads;
    QHash<QPair<QObject *, QObject *>, int> conns;
    //m_connectionModel->refresh();
    for (int row = 0; row < m_connectionModel->rowCount(); ++row) {
        auto sender = m_connectionModel->senderObject(row);
        if (filterObject(sender))
            continue;
        auto receiver = m_connectionModel->receiverObject(row);
        if (filterObject(receiver))
            continue;
        if (!m_objectDiscriminator->isAccepting(sender))
            continue;
        if (!m_objectDiscriminator->isAccepting(receiver))
            continue;
        if (!m_classDiscriminator->isAccepting(sender->metaObject()))
            continue;
        if (!m_classDiscriminator->isAccepting(receiver->metaObject()))
            continue;
        if (!m_threadDiscriminator->isAccepting(sender->thread()))
            continue;
        if (!m_threadDiscriminator->isAccepting(receiver->thread()))
            continue;
        if (sender == receiver)
            continue;
        if (receiver > sender)
            std::swap(sender, receiver);
        threads.insert(sender->thread());
        threads.insert(receiver->thread());
        const auto pair = qMakePair(sender, receiver);
        if (objects.contains(sender) && objects.contains(receiver)) {
            conns[pair]++;
            continue;
        }
        conns.insert(pair, 1);
        objects.insert(sender);
        objects.insert(receiver);
    }

    QVector<VertexItem> vertexItems;
    for (auto object : objects) {
        vertexItems.append({reinterpret_cast<quint64>(object),
                            Util::displayString(object),
                            reinterpret_cast<quint64>(object->thread())});
    }
    m_vertexModel->setVertexItems(vertexItems);

    QVector<ClusterItem> clusterItems;
    for (auto thread : threads) {
        clusterItems.append({reinterpret_cast<quint64>(thread), Util::displayString(thread)});
    }
    m_clusterModel->setClusterItems(clusterItems);

    QVector<EdgeItem> edgeItems(conns.count());
    auto currentEdge = edgeItems.begin();
    auto current = conns.constKeyValueBegin();
    auto none = conns.constKeyValueEnd();
    quint64 id = 0;
    while (current != none) {
        *currentEdge = {id,
                        QString::number(id),
                        reinterpret_cast<quint64>((*current).first.first),
                        reinterpret_cast<quint64>((*current).first.second),
                        (*current).second};
        current++;
        currentEdge++;
        id++;
    }
    m_edgeModel->setEdgeItems(edgeItems);

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
    m_typeDiscriminator = new TypeDiscriminator(typeFilterName(), this);
    m_typeDiscriminator->setDiscriminationRole(ConnectionTypeModel::TypeRole);
    m_typeDiscriminator->setSourceModel(new ConnectionTypeModel(this));
    m_typeDiscriminator->initialise(m_probe);
}

// TOTO: need a refresh method
void AcquisitionEngine::registerConnectionDiscriminator()
{
    m_connectionDiscriminator = new ConnectionDiscriminator(connectionFilterName(), this);
    m_connectionDiscriminator->setDiscriminationRole(ConnectionModel::ConnectionIdRole);
    m_connectionModel = new ConnectionModel(this);
    connect(m_probe, &Probe::objectCreated, m_connectionModel, &ConnectionModel::addObject);
    connect(m_probe, &Probe::objectDestroyed, m_connectionModel, &ConnectionModel::removeObject);
    m_connectionDiscriminator->setSourceModel(m_connectionModel);
    m_connectionDiscriminator->initialise(m_probe);
}

void AcquisitionEngine::registerThreadDiscriminator()
{
    m_threadDiscriminator = new ThreadDiscriminator(threadFilterName(), this);
    m_threadDiscriminator->setDiscriminationRole(ObjectModel::ObjectRole);
    auto input = new ObjectTypeFilterProxyModel<QThread>(this);
    input->setSourceModel(m_probe->objectTreeModel());
    m_threadDiscriminator->setSourceModel(input);
    m_threadDiscriminator->initialise(m_probe);
}

void AcquisitionEngine::registerClassDiscriminator()
{
    m_classDiscriminator = new ClassDiscriminator(classFilterName(), this);
    m_classDiscriminator->setDiscriminationRole(QMetaObjectModel::MetaObjectRole);
    m_classDiscriminator->setSourceModel(m_probe->metaObjectTreeModel());
    m_classDiscriminator->initialise(m_probe);
}

void AcquisitionEngine::registerObjectDiscriminator()
{
    m_objectDiscriminator = new ObjectDiscriminator(objectFilterName(), this);
    m_objectDiscriminator->setDiscriminationRole(ObjectModel::ObjectRole);
    auto input = new SingleColumnObjectProxyModel(this);
    input->setSourceModel(m_probe->objectTreeModel());
    m_objectDiscriminator->setSourceModel(input);
    m_objectDiscriminator->initialise(m_probe);
}

void AcquisitionEngine::registerVertexModel()
{
    m_vertexModel = new Connectivity::VertexModel(this);
    m_probe->registerModel(Connectivity::vertexModelId(), m_vertexModel);
}

void AcquisitionEngine::registerClusterModel()
{
    m_clusterModel = new Connectivity::ClusterModel(this);
    m_probe->registerModel(Connectivity::clusterModelId(), m_clusterModel);
}

void AcquisitionEngine::registerEdgeModel()
{
    m_edgeModel = new Connectivity::EdgeModel(this);
    m_probe->registerModel(Connectivity::edgeModelId(), m_edgeModel);
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
