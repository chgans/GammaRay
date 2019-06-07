#pragma once

#include <QQueue>

#include "acquisitioninterface.h"

// For templates
#include "discriminatorproxymodel.h"
#include <3rdparty/kde/krecursivefilterproxymodel.h>

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
using TypeDiscriminator = Discriminator<int, QSortFilterProxyModel>;
using ObjectDiscriminator = Discriminator<QObject *, KRecursiveFilterProxyModel>;
using ThreadDiscriminator = Discriminator<QThread *, KRecursiveFilterProxyModel>;
using ClassDiscriminator = Discriminator<const QMetaObject *, KRecursiveFilterProxyModel>;

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
    void registerConnectionDiscriminator();
    void registerThreadDiscriminator();
    void registerClassDiscriminator();
    void registerObjectDiscriminator();
    void registerConnectivityModel();

#if 0
    void increaseCountersForObject(QObject *object);
    void decreaseCountersForObject(QObject *object);
#endif

    TypeDiscriminator *m_typeDiscriminator;
    ClassDiscriminator *m_classDiscriminator;
    ObjectDiscriminator *m_objectDiscriminator;
    ThreadDiscriminator *m_threadDiscriminator;

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
