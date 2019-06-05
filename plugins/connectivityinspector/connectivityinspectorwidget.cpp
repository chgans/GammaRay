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
#include "ui_acquisitionwidget.h"
#include "ui_connectivityinspectorwidget.h"

#include "acquisitioncontroller.h"
#include "connectivityinspectorcommon.h"
#include "countdecoratorproxymodel.h"
#include "filtercontroller.h"

#include <common/objectbroker.h>
#include <ui/searchlinecontroller.h>

namespace {
QObject *createObjectVisualizerClient(const QString & /*name*/, QObject *parent)
{
    return new GammaRay::AcquisitionController(parent);
}
} // namespace

using namespace GammaRay;

ConnectivityInspectorWidget::ConnectivityInspectorWidget(QWidget *parent)
    : QWidget(parent), m_ui(new Ui::ObjectVisualizerWidget),
      m_stateManager(this) {
    setupClient();
    setupModels();
    setupUi();
}

ConnectivityInspectorWidget::~ConnectivityInspectorWidget() = default;

void ConnectivityInspectorWidget::setupClient() {
    ObjectBroker::registerClientObjectFactoryCallback<AcquisitionInterface *>(
        createObjectVisualizerClient);
    m_interface = ObjectBroker::object<AcquisitionInterface *>();
}

void ConnectivityInspectorWidget::setupModels() {
    m_connectionModel = ObjectBroker::model(ObjectVisualizerConnectionModelId);
    Q_ASSERT(m_connectionModel != nullptr);

    m_connectionFilterModel =
        ObjectBroker::model(ObjectVisualizerConnectionTypeModelId);
    Q_ASSERT(m_connectionFilterModel != nullptr);
    m_connectionFilterInterface = new FilterController("Connection", this);

    m_threadRecordingModel = ObjectBroker::model(ObjectVisualizerThreadModelId);
    Q_ASSERT(m_threadRecordingModel != nullptr);
    m_threadFilterInterface = new FilterController("Thread", this);

    m_classRecordingModel = ObjectBroker::model(ObjectVisualizerClassModelId);
    Q_ASSERT(m_classRecordingModel != nullptr);
    m_classFilterInterface = new FilterController("Class", this);

    m_objectRecordingModel = ObjectBroker::model(ObjectVisualizerObjectModelId);
    Q_ASSERT(m_objectRecordingModel != nullptr);
    m_objectFilterInterface = new FilterController("Object", this);
}

void ConnectivityInspectorWidget::setupUi() {
    m_ui->setupUi(this);
    setupConnectionView();
    setupFilterWidget(m_ui->connectionFilterWidget, m_connectionFilterInterface,
                      m_connectionFilterModel);
    setupFilterWidget(m_ui->threadFilterWidget, m_threadFilterInterface,
                      m_threadRecordingModel);
    setupFilterWidget(m_ui->classFilterWidget, m_classFilterInterface,
                      m_classRecordingModel);
    setupFilterWidget(m_ui->objectFilterWidget, m_objectFilterInterface,
                      m_objectRecordingModel);
    m_ui->gvWidget->setModel(m_connectionModel);
    m_ui->vtkWidget->setModel(m_connectionModel);

    m_ui->acquisitionWidget->setAcquisitionInterface(m_interface);
}

void ConnectivityInspectorWidget::setupConnectionView() {
    auto view = m_ui->connectionTreeView;
    view->header()->setObjectName("connectionTreeViewHeader");
    new SearchLineController(m_ui->connectionSearchLine, m_connectionModel);
    view->setModel(m_connectionModel);
    view->setDeferredResizeMode(0, QHeaderView::ResizeToContents);
    view->setDeferredResizeMode(1, QHeaderView::ResizeToContents);
    view->setDeferredResizeMode(2, QHeaderView::Stretch);
    view->setSortingEnabled(true);
}

void ConnectivityInspectorWidget::setupFilterWidget(FilterWidget *widget,
                                                    FilterInterface *interface,
                                                    QAbstractItemModel *model) {
    auto proxy = new CountDecoratorProxyModel(this);
    proxy->setSourceModel(model);
    widget->setup(interface, proxy);
}
