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
#include "connectionmodel.h"
#include "connectivityinspectorcommon.h"
#include "countdecoratorproxymodel.h"
#include "filtercontroller.h"
#include "synchonousproxymodel.h"
#include "vtk/vtkgraphadapter.h"

#include <3rdparty/kde/kdescendantsproxymodel.h>
#include <common/objectbroker.h>
#include <common/objectmodel.h>
#include <ui/searchlinecontroller.h>

#include <QTimer>

namespace {
QObject *createObjectVisualizerClient(const QString & /*name*/, QObject *parent)
{
    return new GammaRay::AcquisitionController(parent);
}
} // namespace

using namespace GammaRay;
using namespace GammaRay::Connectivity;

ConnectivityInspectorWidget::ConnectivityInspectorWidget(QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::ObjectVisualizerWidget)
    , m_stateManager(this)
{
    m_ui->setupUi(this);

    ObjectBroker::registerClientObjectFactoryCallback<AcquisitionInterface *>(
        createObjectVisualizerClient);
    m_interface = ObjectBroker::object<AcquisitionInterface *>();

    m_ui->acquisitionWidget->setAcquisitionInterface(m_interface);

    addFilter(typeFilterName(), m_ui->typeFilterWidget);
    addFilter(classFilterName(), m_ui->classFilterWidget);
    addFilter(objectFilterName(), m_ui->objectFilterWidget);
    addFilter(connectionFilterName(), m_ui->connectionFilterWidget);
    addFilter(threadFilterName(), m_ui->threadFilterWidget);

    auto graphAdapter = m_ui->vtkWidget->graphAdapater();

    auto vertexAsyncModel = ObjectBroker::model(vertexModelId());
    auto vertexModel = new SynchonousProxyModel(this);
    vertexModel->addSynchronousRequirement(0, Connectivity::VertexIdRole);
    vertexModel->addSynchronousRequirement(0, Connectivity::LabelRole);
    vertexModel->addSynchronousRequirement(0, Connectivity::ClusterIdRole);
    vertexModel->setSourceModel(vertexAsyncModel);
    auto vertexSelectionModel = new QItemSelectionModel(vertexModel, this);
    new SearchLineController(m_ui->vertexSearchLine, vertexModel);
    m_ui->vertexListView->setModel(vertexModel);
    m_ui->vertexListView->setSelectionModel(vertexSelectionModel);
    graphAdapter->setVertexModel(vertexModel);
    graphAdapter->setVertexSelectionModel(vertexSelectionModel);
    graphAdapter->setVertexIdQuery(0, Connectivity::VertexIdRole);
    graphAdapter->setVertexLabelQuery(0, Connectivity::LabelRole);
    graphAdapter->setVertexClusterIdQuery(0, Connectivity::ClusterIdRole);

    auto clusterAsyncModel = ObjectBroker::model(clusterModelId());
    auto clusterModel = new SynchonousProxyModel(this);
    clusterModel->addSynchronousRequirement(0, Connectivity::ClusterIdRole);
    clusterModel->addSynchronousRequirement(0, Connectivity::LabelRole);
    clusterModel->setSourceModel(clusterAsyncModel);
    auto clusterSelectionModel = new QItemSelectionModel(clusterModel, this);
    new SearchLineController(m_ui->clusterSearchLine, clusterModel);
    m_ui->clusterListView->setModel(clusterModel);
    m_ui->clusterListView->setSelectionModel(clusterSelectionModel);
    graphAdapter->setClusterModel(clusterModel);
    graphAdapter->setClusterSelectionModel(clusterSelectionModel);
    graphAdapter->setClusterIdQuery(0, Connectivity::ClusterIdRole);
    graphAdapter->setClusterLabelQuery(0, Connectivity::LabelRole);

    auto edgeAsyncModel = ObjectBroker::model(edgeModelId());
    auto edgeModel = new SynchonousProxyModel(this);
    edgeModel->addSynchronousRequirement(0, Connectivity::EdgeIdRole);
    edgeModel->addSynchronousRequirement(0, Connectivity::LabelRole);
    edgeModel->addSynchronousRequirement(0, Connectivity::SourceIdRole);
    edgeModel->addSynchronousRequirement(0, Connectivity::TargetIdRole);
    edgeModel->addSynchronousRequirement(0, Connectivity::WeightRole);
    edgeModel->setSourceModel(edgeAsyncModel);
    auto edgeSelectionModel = new QItemSelectionModel(edgeModel, this);
    new SearchLineController(m_ui->edgeSearchLine, edgeModel);
    m_ui->edgeListView->setModel(edgeModel);
    m_ui->edgeListView->setSelectionModel(edgeSelectionModel);
    graphAdapter->setEdgeModel(edgeModel);
    graphAdapter->setEdgeSelectionModel(edgeSelectionModel);
    graphAdapter->setEdgeIdQuery(0, Connectivity::EdgeIdRole);
    graphAdapter->setEdgeLabelQuery(0, Connectivity::LabelRole);
    graphAdapter->setEdgeSourceIdQuery(0, Connectivity::SourceIdRole);
    graphAdapter->setEdgeTargetIdQuery(0, Connectivity::TargetIdRole);
    graphAdapter->setEdgeWeightQuery(0, Connectivity::WeightRole);

    auto timer = new QTimer(this);
    timer->setInterval(2000);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, graphAdapter, &vtkGraphAdapter::recompute);
    connect(m_interface, &AcquisitionInterface::samplingDone, this, [timer]() { timer->start(); });
}

void ConnectivityInspectorWidget::addFilter(const QString name, FilterWidget *widget)
{
    auto model = ObjectBroker::model(filterModelId(name));
    auto interface = new FilterController(name, this);
    widget->setup(interface, model);
}

ConnectivityInspectorWidget::~ConnectivityInspectorWidget() = default;
