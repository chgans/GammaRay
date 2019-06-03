/*
  objectvisualizerwidget.cpp

  This file is part of QGraphViz, a Qt wrapper around GraphViz libraries.

  Copyright (C) 2019

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

#include "objectvisualizerwidget.h"
#include "ui_objectvisualizerwidget.h"

#include "gvcontainer.h"
#include "objectvisualizerclient.h"
#include "objectvisualizercommon.h"
#include "recordingclient.h"
#include "vtkcontainer.h"

#include <common/objectbroker.h>
#include <common/objectmodel.h>
#include <ui/clientdecorationidentityproxymodel.h>
#include <ui/deferredtreeview.h>
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
    assert(m_connectionModel != nullptr);
    m_connectionRecordingModel = ObjectBroker::model(ObjectVisualizerConnectionTypeModelId);
    assert(m_connectionRecordingModel != nullptr);
    //m_objectRecordingInterface = new RecordingClient("Object", this);
    m_threadRecordingModel = ObjectBroker::model(ObjectVisualizerThreadModelId);
    assert(m_threadRecordingModel != nullptr);
    m_threadRecordingInterface = new RecordingClient("Thread", this);
    m_classRecordingModel = ObjectBroker::model(ObjectVisualizerClassModelId);
    assert(m_classRecordingModel != nullptr);
    m_classRecordingInterface = new RecordingClient("Class", this);
    m_objectRecordingModel = ObjectBroker::model(ObjectVisualizerObjectModelId);
    assert(m_objectRecordingModel != nullptr);
    m_objectRecordingInterface = new RecordingClient("Object", this);
}

void ObjectVisualizerWidget::setupUi()
{
    m_ui->setupUi(this);
    setupConnectionView();
    setupConnectionTypeView();
    setupThreadView();
    setupClassView();
    setupObjectView();
    setup2dView();
    setup3dView();
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

void ObjectVisualizerWidget::setupConnectionTypeView()
{
    m_ui->typeTreeView->header()->setObjectName("connectionTypeViewHeader");
    auto typeProxy = new ClientDecorationIdentityProxyModel(this);
    typeProxy->setSourceModel(m_connectionRecordingModel);
    new SearchLineController(m_ui->connectionSearchLine, typeProxy);
    m_ui->typeTreeView->setModel(typeProxy);
    m_ui->typeTreeView->setSortingEnabled(true);
    //    connect(m_ui->recordAllConnectionsButton,
    //            &QToolButton::clicked,
    //            m_interface,
    //            &ObjectVisualizerInterface::recordAll); // FIXME
    //    connect(m_ui->recordNoConnectionsButton,
    //            &QToolButton::clicked,
    //            m_interface,
    //            &ObjectVisualizerInterface::recordNone);
    //    connect(m_ui->showAllConnectionsButton,
    //            &QToolButton::clicked,
    //            m_interface,
    //            &ObjectVisualizerInterface::showAll);
    //    connect(m_ui->showNoConnecctionsButton,
    //            &QToolButton::clicked,
    //            m_interface,
    //            &ObjectVisualizerInterface::showNone);
}

void ObjectVisualizerWidget::setupThreadView()
{
    auto view = m_ui->threadTreeView;
    view->header()->setObjectName("threadViewHeader");
    new SearchLineController(m_ui->threadSearchLine, m_threadRecordingModel);
    view->setDeferredResizeMode(0, QHeaderView::ResizeToContents);
    view->setModel(m_threadRecordingModel);
    view->setSortingEnabled(true);
    connect(m_ui->recordAllThreadsButton,
            &QToolButton::clicked,
            m_threadRecordingInterface,
            &ConnectivityRecordingInterface::recordAll);
    connect(m_ui->recordNoThreadsButton,
            &QToolButton::clicked,
            m_threadRecordingInterface,
            &ConnectivityRecordingInterface::recordNone);
    connect(m_ui->showAllThreadsButton,
            &QToolButton::clicked,
            m_threadRecordingInterface,
            &ConnectivityRecordingInterface::showAll);
    connect(m_ui->showNoThreadsButton,
            &QToolButton::clicked,
            m_threadRecordingInterface,
            &ConnectivityRecordingInterface::showNone);
}

void ObjectVisualizerWidget::setupClassView()
{
    auto view = m_ui->classTreeView;
    view->header()->setObjectName("classViewHeader");
    new SearchLineController(m_ui->threadSearchLine, m_classRecordingModel);
    view->setDeferredResizeMode(0, QHeaderView::ResizeToContents);
    view->setModel(m_classRecordingModel);
    view->setSortingEnabled(true);
    connect(m_ui->recordAllClassesButton,
            &QToolButton::clicked,
            m_classRecordingInterface,
            &ConnectivityRecordingInterface::recordAll);
    connect(m_ui->recordNoClassesButton,
            &QToolButton::clicked,
            m_classRecordingInterface,
            &ConnectivityRecordingInterface::recordNone);
    connect(m_ui->showAllClassesButton,
            &QToolButton::clicked,
            m_classRecordingInterface,
            &ConnectivityRecordingInterface::showAll);
    connect(m_ui->showNoClassesButton,
            &QToolButton::clicked,
            m_classRecordingInterface,
            &ConnectivityRecordingInterface::showNone);
}

void ObjectVisualizerWidget::setupObjectView()
{
    auto view = m_ui->objectTreeView;
    view->header()->setObjectName("objectViewHeader");
    new SearchLineController(m_ui->threadSearchLine, m_objectRecordingModel);
    view->setDeferredResizeMode(0, QHeaderView::ResizeToContents);
    view->setModel(m_objectRecordingModel);
    view->setSortingEnabled(true);
    connect(m_ui->recordAllObjectsButton,
            &QToolButton::clicked,
            m_objectRecordingInterface,
            &ConnectivityRecordingInterface::recordAll);
    connect(m_ui->recordNoObjectsButton,
            &QToolButton::clicked,
            m_objectRecordingInterface,
            &ConnectivityRecordingInterface::recordNone);
    connect(m_ui->showAllObjectsButton,
            &QToolButton::clicked,
            m_objectRecordingInterface,
            &ConnectivityRecordingInterface::showAll);
    connect(m_ui->showNoObjectsButton,
            &QToolButton::clicked,
            m_objectRecordingInterface,
            &ConnectivityRecordingInterface::showNone);
}

void ObjectVisualizerWidget::setup2dView()
{
    m_ui->gvTab->setModel(m_connectionModel);
}

void ObjectVisualizerWidget::setup3dView()
{
    m_ui->vtkTab->setModel(m_connectionModel);
}
