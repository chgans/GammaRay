/*
  vtkgraphadapter.cpp

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

#include "vtkgraphadapter.h"

#include <common/objectid.h>

#include <vtkAnnotationLink.h>
#include <vtkCommand.h>
#include <vtkDataSetAttributes.h>
#include <vtkDoubleArray.h>
#include <vtkMutableDirectedGraph.h>
#include <vtkSelection.h>
#include <vtkSelectionNode.h>
#include <vtkStringArray.h>
#include <vtkTypeUInt64Array.h>

#include <QAbstractTableModel>
#include <QItemSelectionModel>

using namespace GammaRay;

namespace {

const char *sNodeLabelArrayName{"NodeLabel"};
const char *sEdgeLabelArrayName{"EdgeLabel"};
const char *sNodeIdArrayName{"NodeId"};
const char *sEdgeIdArrayName{"EdgeId"};

QString address(quint64 id)
{
    return QStringLiteral("0x%1").arg(id, 8, 16);
}
} // namespace

vtkGraphAdapter::vtkGraphAdapter(QObject *parent)
    : QObject(parent)
{
    m_vertexIds = vtkSmartPointer<vtkTypeUInt64Array>::New();
    m_vertexIds->SetName("VertexId");
    m_vertexLabels = vtkSmartPointer<vtkStringArray>::New();
    m_vertexLabels->SetName("VertexLabel");
    m_vertexClusterIds = vtkSmartPointer<vtkTypeUInt64Array>::New();
    m_vertexClusterIds->SetName("VertexClusterId");

    m_clusterIds = vtkSmartPointer<vtkTypeUInt64Array>::New();
    m_clusterIds->SetName("ClusterId");
    m_clusterLabels = vtkSmartPointer<vtkStringArray>::New();
    m_clusterLabels->SetName("ClusterLabel");

    m_edgeIds = vtkSmartPointer<vtkTypeUInt64Array>::New();
    m_edgeIds->SetName("EdgeId");
    m_edgeLabels = vtkSmartPointer<vtkStringArray>::New();
    m_edgeLabels->SetName("EdgeLabel");
    m_edgeWeights = vtkSmartPointer<vtkDoubleArray>::New();
    m_edgeWeights->SetName("EdgeWeight");

    m_graph = vtkSmartPointer<vtkMutableDirectedGraph>::New();
}

vtkGraphAdapter::~vtkGraphAdapter() {}

void vtkGraphAdapter::setVertexModel(QAbstractItemModel *model)
{
    Q_ASSERT(m_vertexModel == nullptr);
    m_vertexModel = model;
    connectModel(m_vertexModel);
}

void vtkGraphAdapter::setVertexSelectionModel(QItemSelectionModel *model)
{
    Q_ASSERT(m_vertexSelectionModel == nullptr);
    m_vertexSelectionModel = model;
    connect(m_vertexSelectionModel,
            &QItemSelectionModel::selectionChanged,
            this,
            &vtkGraphAdapter::vertexSelectionChanged);
}

void vtkGraphAdapter::setVertexIdQuery(int column, int role)
{
    m_vertexIdColumn = column;
    m_vertexIdRole = role;
}

void vtkGraphAdapter::setVertexLabelQuery(int column, int role)
{
    m_vertexLabelColumn = column;
    m_vertexLabelRole = role;
}

void vtkGraphAdapter::setVertexClusterIdQuery(int column, int role)
{
    m_vertexClusterIdColumn = column;
    m_vertexClusterIdRole = role;
}

void vtkGraphAdapter::setClusterModel(QAbstractItemModel *model)
{
    Q_ASSERT(m_clusterModel == nullptr);
    m_clusterModel = model;
    connectModel(m_clusterModel);
}

void vtkGraphAdapter::setClusterSelectionModel(QItemSelectionModel *model)
{
    Q_ASSERT(m_clusterSelectionModel == nullptr);
    m_clusterSelectionModel = model;
}

void vtkGraphAdapter::setClusterIdQuery(int column, int role)
{
    m_clusterIdColumn = column;
    m_clusterIdRole = role;
}

void vtkGraphAdapter::setClusterLabelQuery(int column, int role)
{
    m_clusterLabelColumn = column;
    m_clusterLabelRole = role;
}

void vtkGraphAdapter::setEdgeModel(QAbstractItemModel *model)
{
    Q_ASSERT(m_edgeModel == nullptr);
    m_edgeModel = model;
    connectModel(m_edgeModel);
}

void vtkGraphAdapter::setEdgeSelectionModel(QItemSelectionModel *model)
{
    Q_ASSERT(m_edgeSelectionModel == nullptr);
    m_edgeSelectionModel = model;
    connect(m_edgeSelectionModel,
            &QItemSelectionModel::selectionChanged,
            this,
            &vtkGraphAdapter::edgeSelectionChanged);
}

void vtkGraphAdapter::setEdgeIdQuery(int column, int role)
{
    m_edgeIdColumn = column;
    m_edgeIdRole = role;
}

void vtkGraphAdapter::setEdgeLabelQuery(int column, int role)
{
    m_edgeLabelColumn = column;
    m_edgeLabelRole = role;
}

void vtkGraphAdapter::setEdgeSourceIdQuery(int column, int role)
{
    m_edgeSourceColumn = column;
    m_edgeSourceRole = role;
}

void vtkGraphAdapter::setEdgeTargetIdQuery(int column, int role)
{
    m_edgeTargetColumn = column;
    m_edgeTargetRole = role;
}

void vtkGraphAdapter::setEdgeWeightQuery(int column, int role)
{
    m_edgeWeightColumn = column;
    m_edgeWeightRole = role;
}

void vtkGraphAdapter::setAnnotationLink(vtkAnnotationLink *link)
{
    Q_ASSERT(m_annotationLink.Get() == nullptr);
    m_annotationLink = link;
    m_annotationLink->AddObserver(vtkCommand::AnnotationChangedEvent,
                                  this,
                                  &vtkGraphAdapter::annotationChangedEvent);
}

void vtkGraphAdapter::recompute()
{
    m_graph->Initialize();

    const int vertexCount = m_vertexModel->rowCount();
    m_vertexIds->SetNumberOfValues(vertexCount);
    m_graph->GetVertexData()->AddArray(m_vertexIds);
    m_vertexLabels->SetNumberOfValues(vertexCount);
    m_graph->GetVertexData()->AddArray(m_vertexLabels);
    m_vertexClusterIds->SetNumberOfValues(vertexCount);
    m_graph->GetVertexData()->AddArray(m_vertexClusterIds);
    m_graph->GetVertexData()->SetPedigreeIds(m_vertexIds);
    for (int index = 0; index < vertexCount; index++) {
        m_vertexIds->SetValue(index, vertexId(index));
        m_vertexLabels->SetValue(index, vertexLabel(index));
        m_vertexClusterIds->SetValue(index, vertexClusterId(index));
        //qDebug() << Q_FUNC_INFO << "Vertex" << index << vertexId(index)
        //         << QString::fromStdString(vertexLabel(index));
    }
    m_graph->SetNumberOfVertices(vertexCount);

    const int edgeCount = m_edgeModel->rowCount();
    m_edgeIds->SetNumberOfValues(edgeCount);
    m_graph->GetEdgeData()->AddArray(m_edgeIds);
    m_edgeLabels->SetNumberOfValues(edgeCount);
    m_graph->GetEdgeData()->AddArray(m_edgeLabels);
    m_edgeWeights->SetNumberOfValues(edgeCount);
    m_graph->GetEdgeData()->AddArray(m_edgeWeights);
    m_graph->GetEdgeData()->SetPedigreeIds(m_edgeIds);
    int ignored = 0;
    for (int index = 0; index < edgeCount; index++) {
        m_edgeIds->SetValue(index, edgeId(index));
        m_edgeLabels->SetValue(index, edgeLabel(index));
        m_edgeWeights->SetValue(index, edgeWeight(index));
        const auto sourceId = m_graph->FindVertex(edgeSourceId(index));
        const auto targetId = m_graph->FindVertex(edgeTargetId(index));
        if (sourceId <= 0 || targetId <= 0) {
            qWarning() << Q_FUNC_INFO << "Ignoring dodgy edge:" << address(edgeId(index));
            ignored++;
            continue;
        }
        m_graph->AddEdge(sourceId, targetId);
        //qDebug() << Q_FUNC_INFO << "Edge" << index << edgeId(index) << sourceId << targetId;
    }
    m_edgeIds->SetNumberOfValues(edgeCount - ignored);
    m_edgeLabels->SetNumberOfValues(edgeCount - ignored);
    m_edgeWeights->SetNumberOfValues(edgeCount - ignored);

    qDebug() << Q_FUNC_INFO << m_graph->GetNumberOfVertices() << m_graph->GetNumberOfEdges();

    emit graphChanged();
}

void vtkGraphAdapter::vertexSelectionChanged(const QItemSelection &current,
                                             const QItemSelection &previous)
{
    Q_UNUSED(current);
    Q_UNUSED(previous);
    updateAnnotation();
}

void vtkGraphAdapter::edgeSelectionChanged(const QItemSelection &current,
                                           const QItemSelection &previous)
{
    Q_UNUSED(current);
    Q_UNUSED(previous);
    updateAnnotation();
}

void vtkGraphAdapter::annotationChangedEvent()
{
    qDebug() << Q_FUNC_INFO;
    auto selection = m_annotationLink->GetCurrentSelection();
    Q_ASSERT(selection->GetNumberOfNodes() == 2);
    auto node0 = selection->GetNode(0);
    auto node1 = selection->GetNode(1);
    if (node0->GetFieldType() == vtkSelectionNode::EDGE)
        std::swap(node0, node1);
    Q_ASSERT(node0->GetFieldType() == vtkSelectionNode::VERTEX);
    Q_ASSERT(node0->GetContentType() == vtkSelectionNode::PEDIGREEIDS);
    auto vertexList = node0->GetSelectionList();
    Q_ASSERT(node1->GetFieldType() == vtkSelectionNode::EDGE);
    Q_ASSERT(node1->GetContentType() == vtkSelectionNode::PEDIGREEIDS);
    auto edgeList = node1->GetSelectionList();

    QSet<quint64> vertexSet;
    for (int i = 0; i < vertexList->GetNumberOfValues(); ++i) {
        const auto id = vertexList->GetVariantValue(i).ToTypeUInt64();
        vertexSet.insert(id);
        qDebug() << "Vertex selected: " << QString::number(id, 16);
    }
    QItemSelection vertexItemSelection;
    for (int row = 0; row < m_vertexModel->rowCount(); ++row) {
        const auto index = m_vertexModel->index(row, 0);
        if (vertexSet.contains(vertexId(row))) {
            vertexItemSelection.select(index, index);
        }
    }
    m_vertexSelectionModel->select(vertexItemSelection,
                                   QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

    QSet<quint64> edgeSet;
    for (int i = 0; i < edgeList->GetNumberOfValues(); ++i) {
        const auto id = edgeList->GetVariantValue(i).ToTypeUInt64();
        edgeSet.insert(id);
        qDebug() << "Edge selected: " << QString::number(id, 16);
    }
    QItemSelection edgeItemSelection;
    for (int row = 0; row < m_edgeModel->rowCount(); ++row) {
        const auto index = m_edgeModel->index(row, 0);
        if (edgeSet.contains(edgeId(row))) {
            edgeItemSelection.select(index, index);
        }
    }
    m_edgeSelectionModel->select(edgeItemSelection,
                                 QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
}

void vtkGraphAdapter::updateAnnotation()
{
    auto selection = vtkSelection::New();
    auto node = vtkSelectionNode::New();
    node->SetFieldType(vtkSelectionNode::VERTEX);
    node->SetContentType(vtkSelectionNode::PEDIGREEIDS);
    auto list = vtkTypeUInt64Array::New();
    for (const auto &index : m_vertexSelectionModel->selectedIndexes()) {
        list->InsertNextValue(vertexId(index.row()));
    }
    node->SetSelectionList(list);
    selection->AddNode(node);

    node = vtkSelectionNode::New();
    node->SetFieldType(vtkSelectionNode::EDGE);
    node->SetContentType(vtkSelectionNode::PEDIGREEIDS);
    list = vtkTypeUInt64Array::New();
    for (const auto &index : m_edgeSelectionModel->selectedIndexes()) {
        list->InsertNextValue(edgeId(index.row()));
    }
    node->SetSelectionList(list);
    selection->AddNode(node);

    m_annotationLink->SetCurrentSelection(selection);
}

vtkGraph *vtkGraphAdapter::graph()
{
    return m_graph;
}

quint64 vtkGraphAdapter::vertexId(int index)
{
    Q_ASSERT(index >= 0);
    Q_ASSERT(m_vertexIdColumn >= 0);
    Q_ASSERT(m_vertexIdRole >= 0);
    const QModelIndex modelIndex = m_vertexModel->index(index, m_vertexIdColumn);
    const auto variant = modelIndex.data(m_vertexIdRole);
    Q_ASSERT(variant.isValid());
    Q_ASSERT(variant.canConvert<quint64>());
    return variant.value<quint64>();
}

std::string vtkGraphAdapter::vertexLabel(int index)
{
    Q_ASSERT(index >= 0);
    Q_ASSERT(m_vertexLabelColumn >= 0);
    Q_ASSERT(m_vertexLabelRole >= 0);
    const QModelIndex modelIndex = m_vertexModel->index(index, m_vertexLabelColumn);
    const auto variant = modelIndex.data(m_vertexLabelRole);
    Q_ASSERT(variant.isValid());
    Q_ASSERT(variant.canConvert<QString>());
    return variant.toString().toStdString();
}

quint64 vtkGraphAdapter::vertexClusterId(int index)
{
    Q_ASSERT(index >= 0);
    Q_ASSERT(m_vertexClusterIdColumn >= 0);
    Q_ASSERT(m_vertexClusterIdRole >= 0);
    const QModelIndex modelIndex = m_vertexModel->index(index, m_vertexClusterIdColumn);
    const auto variant = modelIndex.data(m_vertexClusterIdRole);
    Q_ASSERT(variant.isValid());
    Q_ASSERT(variant.canConvert<quint64>());
    return variant.value<quint64>();
}

quint64 vtkGraphAdapter::edgeId(int index)
{
    Q_ASSERT(index >= 0);
    Q_ASSERT(m_edgeIdColumn >= 0);
    Q_ASSERT(m_edgeIdRole >= 0);
    const QModelIndex modelIndex = m_edgeModel->index(index, m_edgeIdColumn);
    const auto variant = modelIndex.data(m_edgeIdRole);
    Q_ASSERT(variant.isValid());
    Q_ASSERT(variant.canConvert<quint64>());
    return variant.value<quint64>();
}

std::string vtkGraphAdapter::edgeLabel(int index)
{
    Q_ASSERT(index >= 0);
    Q_ASSERT(m_edgeLabelColumn >= 0);
    Q_ASSERT(m_edgeLabelRole >= 0);
    const QModelIndex modelIndex = m_edgeModel->index(index, m_edgeLabelColumn);
    const auto variant = modelIndex.data(m_edgeLabelRole);
    Q_ASSERT(variant.isValid());
    Q_ASSERT(variant.canConvert<QString>());
    return variant.toString().toStdString();
}

quint64 vtkGraphAdapter::edgeSourceId(int index)
{
    Q_ASSERT(index >= 0);
    Q_ASSERT(m_edgeSourceColumn >= 0);
    Q_ASSERT(m_edgeSourceRole >= 0);
    const QModelIndex modelIndex = m_edgeModel->index(index, m_edgeSourceColumn);
    const auto variant = modelIndex.data(m_edgeSourceRole);
    Q_ASSERT(variant.isValid());
    Q_ASSERT(variant.canConvert<quint64>());
    return variant.value<quint64>();
}

quint64 vtkGraphAdapter::edgeTargetId(int index)
{
    Q_ASSERT(index >= 0);
    Q_ASSERT(m_edgeTargetColumn >= 0);
    Q_ASSERT(m_edgeTargetRole >= 0);
    const QModelIndex modelIndex = m_edgeModel->index(index, m_edgeTargetColumn);
    const auto variant = modelIndex.data(m_edgeTargetRole);
    Q_ASSERT(variant.isValid());
    Q_ASSERT(variant.canConvert<quint64>());
    return variant.value<quint64>();
}

double vtkGraphAdapter::edgeWeight(int index)
{
    Q_ASSERT(index >= 0);
    Q_ASSERT(m_edgeWeightColumn >= 0);
    Q_ASSERT(m_edgeWeightRole >= 0);
    const QModelIndex modelIndex = m_edgeModel->index(index, m_edgeWeightColumn);
    const auto variant = modelIndex.data(m_edgeWeightRole);
    Q_ASSERT(variant.isValid());
    Q_ASSERT(variant.canConvert<double>());
    return variant.value<double>();
}

void vtkGraphAdapter::connectModel(QAbstractItemModel *model)
{
    //    connect(model, &QAbstractItemModel::rowsInserted, this, &vtkGraphAdapter::recompute);
    //    connect(model, &QAbstractItemModel::rowsRemoved, this, &vtkGraphAdapter::recompute);
    //    connect(model, &QAbstractItemModel::dataChanged, this, &vtkGraphAdapter::recompute);
    //    connect(model, &QAbstractItemModel::modelReset, this, &vtkGraphAdapter::recompute);
}
