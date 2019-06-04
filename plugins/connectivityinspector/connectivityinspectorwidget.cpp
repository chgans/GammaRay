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

#include "connectivityinspectorwidget.h"
#include "ui_connectivityinspectorwidget.h"

#include "connectivityinspectorclient.h"
#include "connectivityinspectorcommon.h"
#include "countdecoratorproxymodel.h"
#include "recordingclient.h"

#include <common/objectbroker.h>
#include <ui/searchlinecontroller.h>

namespace {
QObject *createObjectVisualizerClient(const QString & /*name*/, QObject *parent)
{
    return new GammaRay::ConnectivityInspectorClient(parent);
}
} // namespace

using namespace GammaRay;

ObjectVisualizerWidget::ObjectVisualizerWidget(QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::ObjectVisualizerWidget)
    , m_stateManager(this)
{
    setupClient();
    setupModels();
    setupUi();
}

ObjectVisualizerWidget::~ObjectVisualizerWidget() = default;

void ObjectVisualizerWidget::setupClient()
{
    ObjectBroker::registerClientObjectFactoryCallback<ConnectivityInspectorInterface *>(
        createObjectVisualizerClient);
    m_interface = ObjectBroker::object<ConnectivityInspectorInterface *>();
}

void ObjectVisualizerWidget::setupModels()
{
    m_connectionModel = ObjectBroker::model(ObjectVisualizerConnectionModelId);
    Q_ASSERT(m_connectionModel != nullptr);

    m_connectionRecordingModel =
        ObjectBroker::model(ObjectVisualizerConnectionTypeModelId);
    Q_ASSERT(m_connectionRecordingModel != nullptr);
    m_connectionRecordingInterface = new RecordingClient("Connection", this);

    m_threadRecordingModel = ObjectBroker::model(ObjectVisualizerThreadModelId);
    Q_ASSERT(m_threadRecordingModel != nullptr);
    m_threadRecordingInterface = new RecordingClient("Thread", this);

    m_classRecordingModel = ObjectBroker::model(ObjectVisualizerClassModelId);
    Q_ASSERT(m_classRecordingModel != nullptr);
    m_classRecordingInterface = new RecordingClient("Class", this);

    m_objectRecordingModel = ObjectBroker::model(ObjectVisualizerObjectModelId);
    Q_ASSERT(m_objectRecordingModel != nullptr);
    m_objectRecordingInterface = new RecordingClient("Object", this);
}

void ObjectVisualizerWidget::setupUi()
{
    m_ui->setupUi(this);
    setupConnectionView();
    setupRecordingWidget(m_ui->connTypesTab, m_connectionRecordingInterface,
                         m_connectionRecordingModel);
    setupRecordingWidget(m_ui->threadTab, m_threadRecordingInterface,
                         m_threadRecordingModel);
    setupRecordingWidget(m_ui->classTab, m_classRecordingInterface,
                         m_classRecordingModel);
    setupRecordingWidget(m_ui->objectTab, m_objectRecordingInterface,
                         m_objectRecordingModel);
    m_ui->gvTab->setModel(m_connectionModel);
    m_ui->vtkTab->setModel(m_connectionModel);
}

void ObjectVisualizerWidget::setupConnectionView()
{
    auto view = m_ui->connectionTreeView;
    view->header()->setObjectName("connectionTreeViewHeader");
    new SearchLineController(m_ui->connectionSearchLine, m_connectionModel);
    view->setModel(m_connectionModel);
    view->setDeferredResizeMode(0, QHeaderView::ResizeToContents);
    view->setDeferredResizeMode(1, QHeaderView::ResizeToContents);
    view->setDeferredResizeMode(2, QHeaderView::Stretch);
    view->setSortingEnabled(true);
    connect(m_ui->pauseButton,
            &QToolButton::toggled,
            m_interface,
            &ConnectivityInspectorInterface::setIsPaused);
    connect(m_ui->clearButton,
            &QToolButton::clicked,
            m_interface,
            &ConnectivityInspectorInterface::clearHistory);
}

void ObjectVisualizerWidget::setupRecordingWidget(
    RecordingWidget *widget, ConnectivityRecordingInterface *interface,
    QAbstractItemModel *model) {
    auto proxy = new CountDecoratorProxyModel(this);
    proxy->setSourceModel(model);
    widget->setup(interface, proxy);
}
