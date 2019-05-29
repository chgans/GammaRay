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

#ifndef GAMMARAY_OBJECTVISUALIZER_OBJECTVISUALIZER_H
#define GAMMARAY_OBJECTVISUALIZER_OBJECTVISUALIZER_H

#include "objectvisualizerinterface.h"

#include "recordingproxymodel.h"

#include <core/toolfactory.h>

#include <common/objectmodel.h>
#include <common/tools/metaobjectbrowser/qmetaobjectmodel.h>

#include <QQueue>

QT_BEGIN_NAMESPACE
class QTimer;
class QAbstractProxyModel;
QT_END_NAMESPACE

namespace GammaRay {

class ConnectionTypeModel;
class ConnectionModel;

class ConnectivityAnalyser : public ObjectVisualizerInterface
{
    Q_OBJECT

public:
    explicit ConnectivityAnalyser(Probe *probe, QObject *parent = nullptr);
    ~ConnectivityAnalyser() override;

    // ObjectVisualizerInterface interface
public slots:
    void clearHistory() override;
    void recordAll() override;
    void recordNone() override;
    void showAll() override;
    void showNone() override;

private:
    void registerConnectionTypeModel();
    void registerConnectionModel();
    void registerThreadModel();
    void registerClassModel();
    void registerObjectModel();
    void initialise();

    void scheduleAddObject(QObject *object);
    void scheduleRemoveObject(QObject *object);

    void addObject(QObject *object);
    void removeObject(QObject *object);

private slots:
    void processPendingChanges();
    void refine();

private:
    Probe *m_probe;
    ConnectionModel *m_connectionModel;
    ConnectionTypeModel *m_connectionTypeModel;
    RecordingProxyModel<QObject *, ObjectModel::ObjectRole> *m_threadModel;
    RecordingProxyModel<const QMetaObject *, QMetaObjectModel::MetaObjectRole> *m_classModel;
    RecordingProxyModel<QObject *, ObjectModel::ObjectRole> *m_objectModel;
    QTimer *m_updateTimer;
    QQueue<QObject *> m_objectAdded;
    QQueue<QObject *> m_objectRemoved;
};

class ConnectivityAnalyserFactory : public QObject,
                                    public StandardToolFactory<QObject, ConnectivityAnalyser>
{
    Q_OBJECT
    Q_INTERFACES(GammaRay::ToolFactory)
    Q_PLUGIN_METADATA(IID "com.kdab.GammaRay.ToolFactory" FILE "gammaray_objectvisualizer.json")

public:
    explicit ConnectivityAnalyserFactory(QObject *parent = nullptr)
        : QObject(parent)
    {}
};
} // namespace GammaRay

#endif // GAMMARAY_GRAPHVIEWER_H
