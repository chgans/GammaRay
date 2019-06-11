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

namespace {
QObject *createObjectVisualizerClient(const QString & /*name*/, QObject *parent)
{
    return new GammaRay::AcquisitionController(parent);
}
} // namespace

using namespace GammaRay;
using namespace GammaRay::CI;

Filter::Filter(FilterWidget *filterWidget,
               QLineEdit *searchWidget,
               QAbstractItemView *searchableView,
               const QString &name,
               QObject *parent)
{
    auto filteredModel = ObjectBroker::model(filteredModelId(name));
    Q_ASSERT(filteredModel);
    // auto descendantsModel = new KDescendantsProxyModel(parent);
    // descendantsModel->setSourceModel(filteredModel);
    outputModel = new SynchonousProxyModel(parent);
    outputModel->setSourceModel(filteredModel);
    searchableModel = new QSortFilterProxyModel(parent);
    searchableModel->setSourceModel(outputModel);
    new SearchLineController(searchWidget, searchableModel);
    searchableView->setModel(outputModel);

    filterModel = ObjectBroker::model(filterModelId(name));
    Q_ASSERT(filterModel);

    filterInterface = new FilterController(name, parent);

    auto proxy = new CountDecoratorProxyModel(parent);
    proxy->setSourceModel(filterModel);
    filterWidget->setup(filterInterface, proxy);
}

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

#if 0
    m_filters.insert(typeFilterName(),
                     Filter(m_ui->typeFilterWidget, typeFilterName(), this));

    m_filters.insert(classFilterName(),
                     Filter(m_ui->classFilterWidget, classFilterName(), this));
#endif
    m_filters.insert(objectFilterName(),
                     Filter(m_ui->objectFilterWidget,
                            m_ui->vertexSearchLine,
                            m_ui->vertexListView,
                            objectFilterName(),
                            this));
    auto syncModel = m_filters[objectFilterName()].outputModel;
    syncModel->addSynchronousRequirement(0, ObjectModel::ObjectIdRole);

    m_filters.insert(connectionFilterName(),
                     Filter(m_ui->connectionFilterWidget,
                            m_ui->edgeSearchLine,
                            m_ui->edgeListView,
                            connectionFilterName(),
                            this));
    syncModel = m_filters[connectionFilterName()].outputModel;
    syncModel->addSynchronousRequirement(ConnectionModel::ConnectionColumn, ConnectionModel::ConnectionIdRole);
    syncModel->addSynchronousRequirement(ConnectionModel::SenderColumn, ConnectionModel::SenderObjectIdRole);
    syncModel->addSynchronousRequirement(ConnectionModel::ReceiverColumn, ConnectionModel::ReceiverObjectIdRole);

#if 0
    m_filters.insert(threadFilterName(),
                     Filter(m_ui->threadFilterWidget, threadFilterName(), this));
#endif

    auto graphAdapter = new vtkGraphAdapter(this);
    graphAdapter->setVertexIdQuery(0, ObjectModel::ObjectIdRole);
    graphAdapter->setVertexLabelQuery(0, Qt::DisplayRole);
    graphAdapter->setEdgeIdQuery(ConnectionModel::ConnectionColumn,
                                 ConnectionModel::ConnectionIdRole);
    graphAdapter->setEdgeLabelQuery(ConnectionModel::ConnectionColumn, Qt::DisplayRole);
    graphAdapter->setEdgeSourceIdQuery(ConnectionModel::SenderColumn,
                                       ConnectionModel::SenderObjectIdRole);
    graphAdapter->setEdgeTargetIdQuery(ConnectionModel::ReceiverColumn,
                                       ConnectionModel::ReceiverObjectIdRole);

    auto vertexModel = m_filters.value(objectFilterName()).outputModel;
    m_ui->vertexListView->setModel(vertexModel);
    graphAdapter->setVertexModel(vertexModel);
    auto edgeModel = m_filters.value(connectionFilterName()).outputModel;
    m_ui->edgeListView->setModel(edgeModel);
    graphAdapter->setEdgeModel(edgeModel);
    m_ui->vtkWidget->setGraph(graphAdapter->graph());

    //m_ui->gvWidget->setModel(m_connectionModel);
    //m_ui->vtkWidget->setModel(m_connectionModel);

    connect(m_interface, &AcquisitionInterface::samplingDone, this, [this, graphAdapter]() {
        graphAdapter->setup();
        m_ui->vtkWidget->updateGraph();
    });
}

ConnectivityInspectorWidget::~ConnectivityInspectorWidget() = default;
