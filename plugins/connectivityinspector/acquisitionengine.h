#pragma once

#include <QQueue>

#include "acquisitioninterface.h"

// For role enums
#include "connectiontypemodel.h"

// For role enums
#include <common/objectmodel.h>
#include <common/tools/metaobjectbrowser/qmetaobjectmodel.h>

QT_BEGIN_NAMESPACE
class QTimer;
class QObject;
QT_END_NAMESPACE

namespace GammaRay {

class Probe;
class ConnectionModel;
template<class T, int>
class DiscriminatorProxyModel;

class AcquisitionEngine : public AcquisitionInterface {
    Q_OBJECT
public:
    AcquisitionEngine(Probe *probe, QObject *parent);
    ~AcquisitionEngine() override;

    // AcquisitionInterface interface
public:
    State state() const override;
    int bufferSize() const override;
    qreal bufferUsage() const override;
    int bufferOverrunCount() const override;
    qreal samplingRate() const override;

public slots:
    void start() override;
    void stop() override;
    void pause() override;
    void resume() override;
    void refresh() override;
    void clear() override;
    void setBufferSize(int size) override;
    void setSamplingRate(qreal rate) override;

private:
    void registerConnectionFilterModel();
    void registerThreadFilterModel();
    void registerClassFilterModel();
    void registerObjectFilterModel();
    void registerConnectionModel();

    void increaseCountersForObject(QObject *object);
    void decreaseCountersForObject(QObject *object);

    // clang-format off
    QAbstractItemModel *m_connectionInputModel;
    DiscriminatorProxyModel<int, ConnectionTypeModel::TypeRole> *m_connectionFilterModel;
    QAbstractItemModel *m_threadInputModel;
    DiscriminatorProxyModel<QObject *, ObjectModel::ObjectRole> *m_threadFilterModel;
    QAbstractItemModel *m_classInputModel;
    DiscriminatorProxyModel<const QMetaObject *, QMetaObjectModel::MetaObjectRole> *m_classFilterModel;
    QAbstractItemModel *m_objectInputModel;
    DiscriminatorProxyModel<QObject *, ObjectModel::ObjectRole> *m_objectFilterModel;
    // clang-format on

    void startSampling();
    void stopSampling();
    void pauseSampling();
    void resumeSampling();
    void takeSample();
    void clearSamples();

    int samplingPeriodMs() const;

    struct Connection {};
    QTimer *m_timer;
    QQueue<Connection> m_connectionQueue;
    int m_bufferSize = 0;
    int m_bufferOverrunCount = 0;
    qreal m_samplingRate = 0;
    bool m_isPaused = false;

    Probe *m_probe;
    ConnectionModel *m_connectionModel;
    bool isValidObject(QObject *sender);
    void sampleObject(QObject *sender);
};

} // namespace GammaRay
