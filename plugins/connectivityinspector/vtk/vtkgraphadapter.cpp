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

#include <vtkDataSetAttributes.h>
#include <vtkMutableDirectedGraph.h>
#include <vtkStringArray.h>
#include <vtkTypeUInt64Array.h>

#include <QAbstractTableModel>

using namespace GammaRay;

namespace {
QString address(quint64 id)
{
    return QStringLiteral("0x%1").arg(id, 8, 16);
}
} // namespace

struct vtkGraphAdapter::DataSource {
    int column;
    int role;
    const QAbstractItemModel *model = nullptr;

    template <class T> T query(int index) const {
        return model->data(model->index(index, column), role).value<T>();
    }
};

vtkGraphAdapter::vtkGraphAdapter(QObject *parent)
    : QObject(parent)
{
    m_vertexIds = vtkTypeUInt64Array::New();
    m_vertexIds->SetName("NodeId");
    m_vertexLabels = vtkStringArray::New();
    m_vertexLabels->SetName("NodeLabel");
    m_edgeIds = vtkTypeUInt64Array::New();
    m_edgeIds->SetName("EdgeId");
    m_edgeLabels = vtkStringArray::New();
    m_edgeLabels->SetName("EdgeLabel");
    m_graph = vtkMutableDirectedGraph::New();
}

vtkGraphAdapter::~vtkGraphAdapter() {}

void vtkGraphAdapter::setVertexModel(const QAbstractItemModel *model)
{
    Q_ASSERT(m_vertexModel == nullptr);
    m_vertexModel = model;
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

void vtkGraphAdapter::setEdgeModel(const QAbstractItemModel *model)
{
    Q_ASSERT(m_edgeModel == nullptr);
    m_edgeModel = model;
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

void vtkGraphAdapter::setup()
{
    m_graph->Initialize();

    const int vertexCount = m_vertexModel->rowCount();
    m_vertexIds->SetNumberOfValues(vertexCount);
    m_graph->GetVertexData()->AddArray(m_vertexIds);
    m_vertexLabels->SetNumberOfValues(vertexCount);
    m_graph->GetVertexData()->AddArray(m_vertexLabels);
    m_graph->GetVertexData()->SetPedigreeIds(m_vertexIds);
    for (int index = 0; index < vertexCount; index++) {
        m_vertexIds->SetValue(index, vertexId(index));
        m_vertexLabels->SetValue(index, vertexLabel(index));
        qDebug() << Q_FUNC_INFO << "Vertex" << vertexId(index)
                 << QString::fromStdString(vertexLabel(index));
    }
    m_graph->SetNumberOfVertices(vertexCount);

    const int edgeCount = m_edgeModel->rowCount();
    m_edgeIds->SetNumberOfValues(edgeCount);
    m_graph->GetEdgeData()->AddArray(m_edgeIds);
    m_edgeLabels->SetNumberOfValues(edgeCount);
    m_graph->GetEdgeData()->AddArray(m_edgeLabels);
    m_graph->GetEdgeData()->SetPedigreeIds(m_edgeIds);
    for (int index = 0; index < edgeCount; index++) {
        m_edgeIds->SetValue(index, edgeId(index));
        m_edgeLabels->SetValue(index, edgeLabel(index));
        const auto sourceId = edgeSourceId(index);
        const auto targetId = edgeTargetId(index);
        if (m_graph->FindVertex(sourceId) == -1) {
            qWarning() << Q_FUNC_INFO << "Ignoring dodgy edge source:" << address(edgeId(index))
                       << address(sourceId) << targetId;
            continue;
        }
        if (m_graph->FindVertex(targetId) == -1) {
            qWarning() << Q_FUNC_INFO << "Ignoring dodgy edge target:" << address(edgeId(index))
                       << address(targetId) << targetId;
            continue;
        }
        if (!sourceId || !targetId) {
            qWarning() << Q_FUNC_INFO << "Ignoring dodgy edge:" << address(edgeId(index))
                       << address(sourceId) << targetId;
            continue;
        }
        m_graph->AddEdge(vtkVariant(sourceId), vtkVariant(targetId));
    }
    qDebug() << Q_FUNC_INFO << m_graph->GetNumberOfVertices() << m_graph->GetNumberOfEdges();
}

vtkGraph *vtkGraphAdapter::graph()
{
    return m_graph;
}

quint64 vtkGraphAdapter::vertexId(int index)
{
    const QModelIndex modelIndex = m_vertexModel->index(index, m_vertexIdColumn);
    return modelIndex.data(m_vertexIdRole).value<ObjectId>().id();
}

std::string vtkGraphAdapter::vertexLabel(int index)
{
    const QModelIndex modelIndex = m_vertexModel->index(index, m_vertexLabelColumn);
    return modelIndex.data(m_vertexLabelRole).toString().toStdString();
}

quint64 vtkGraphAdapter::edgeId(int index)
{
    const QModelIndex modelIndex = m_edgeModel->index(index, m_edgeIdColumn);
    return modelIndex.data(m_edgeIdRole).value<ObjectId>().id();
}

std::string vtkGraphAdapter::edgeLabel(int index)
{
    const QModelIndex modelIndex = m_edgeModel->index(index, m_edgeLabelColumn);
    return modelIndex.data(m_edgeLabelRole).toString().toStdString();
}

quint64 vtkGraphAdapter::edgeSourceId(int index)
{
    const QModelIndex modelIndex = m_edgeModel->index(index, m_edgeSourceColumn);
    return modelIndex.data(m_edgeSourceRole).value<ObjectId>().id();
}

quint64 vtkGraphAdapter::edgeTargetId(int index)
{
    const QModelIndex modelIndex = m_edgeModel->index(index, m_edgeTargetColumn);
    return modelIndex.data(m_edgeTargetRole).value<ObjectId>().id();
}
