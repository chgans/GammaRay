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
#include "vtkcontainer.h"

#include <common/objectbroker.h>
#include <common/objectmodel.h>
#include <ui/clientdecorationidentityproxymodel.h>
#include <ui/deferredtreeview.h>
#include <ui/searchlinecontroller.h>

namespace {
QObject *createObjectVisualizerClient(const QString & /*name*/, QObject *parent)
{
    return new GammaRay::ObjectVisualizerClient(parent);
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
    ObjectBroker::registerClientObjectFactoryCallback<ObjectVisualizerInterface *>(
        createObjectVisualizerClient);
    m_interface = ObjectBroker::object<ObjectVisualizerInterface *>();
}

void ObjectVisualizerWidget::setupModels()
{
    m_connectionModel = ObjectBroker::model(ObjectVisualizerConnectionModelId);
    assert(m_connectionModel != nullptr);
    m_connectionTypeModel = ObjectBroker::model(ObjectVisualizerConnectionTypeModelId);
    assert(m_connectionTypeModel != nullptr);
    m_threadModel = ObjectBroker::model(ObjectVisualizerThreadModelId);
    assert(m_threadModel != nullptr);
    m_classModel = ObjectBroker::model(ObjectVisualizerClassModelId);
    assert(m_classModel != nullptr);
    m_objectModel = ObjectBroker::model(ObjectVisualizerObjectModelId);
    assert(m_objectModel != nullptr);
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
            &ObjectVisualizerInterface::setIsPaused);
    connect(m_ui->clearButton,
            &QToolButton::clicked,
            m_interface,
            &ObjectVisualizerInterface::clearHistory);
}

void ObjectVisualizerWidget::setupConnectionTypeView()
{
    m_ui->typeTreeView->header()->setObjectName("connectionTypeViewHeader");
    auto typeProxy = new ClientDecorationIdentityProxyModel(this);
    typeProxy->setSourceModel(m_connectionTypeModel);
    new SearchLineController(m_ui->typeSearchLine, typeProxy);
    m_ui->typeTreeView->setModel(typeProxy);
    m_ui->typeTreeView->setSortingEnabled(true);
    connect(m_ui->recordAllButton,
            &QToolButton::clicked,
            m_interface,
            &ObjectVisualizerInterface::recordAll);
    connect(m_ui->recordNoneButton,
            &QToolButton::clicked,
            m_interface,
            &ObjectVisualizerInterface::recordNone);
    connect(m_ui->showAllButton,
            &QToolButton::clicked,
            m_interface,
            &ObjectVisualizerInterface::showAll);
    connect(m_ui->showNoneButton,
            &QToolButton::clicked,
            m_interface,
            &ObjectVisualizerInterface::showNone);
}

void ObjectVisualizerWidget::setupThreadView()
{
    auto view = m_ui->threadTreeView;
    view->header()->setObjectName("threadViewHeader");
    new SearchLineController(m_ui->threadSearchLine, m_threadModel);
    view->setDeferredResizeMode(0, QHeaderView::ResizeToContents);
    view->setModel(m_threadModel);
    view->setSortingEnabled(true);
#if 0 // TODO
    connect(m_ui->recordAllButton,
            &QToolButton::clicked,
            m_interface,
            &ObjectVisualizerInterface::recordAll);
    connect(m_ui->recordNoneButton,
            &QToolButton::clicked,
            m_interface,
            &ObjectVisualizerInterface::recordNone);
    connect(m_ui->showAllButton,
            &QToolButton::clicked,
            m_interface,
            &ObjectVisualizerInterface::showAll);
    connect(m_ui->showNoneButton,
            &QToolButton::clicked,
            m_interface,
            &ObjectVisualizerInterface::showNone);
#endif
}

void ObjectVisualizerWidget::setupClassView()
{
    auto view = m_ui->classTreeView;
    view->header()->setObjectName("classViewHeader");
    new SearchLineController(m_ui->threadSearchLine, m_classModel);
    view->setDeferredResizeMode(0, QHeaderView::ResizeToContents);
    view->setModel(m_classModel);
    view->setSortingEnabled(true);
}

void ObjectVisualizerWidget::setupObjectView()
{
    auto view = m_ui->objectTreeView;
    view->header()->setObjectName("objectViewHeader");
    new SearchLineController(m_ui->threadSearchLine, m_objectModel);
    view->setDeferredResizeMode(0, QHeaderView::ResizeToContents);
    view->setModel(m_objectModel);
    view->setSortingEnabled(true);
}

void ObjectVisualizerWidget::setup2dView()
{
    m_ui->gvTab->setModel(m_connectionModel);
}

void ObjectVisualizerWidget::setup3dView()
{
    m_ui->vtkTab->setModel(m_connectionModel);
}
