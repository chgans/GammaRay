/*
  vtkgraphadapter.h

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

#ifndef VTKGRAPHADAPTER_H
#define VTKGRAPHADAPTER_H

#include <QObject>

#include <vtkSmartPointer.h>

QT_BEGIN_NAMESPACE
class QAbstractItemModel;
class QItemSelectionModel;
class QItemSelection;
QT_END_NAMESPACE

class vtkDoubleArray;
class vtkGraph;
class vtkMutableDirectedGraph;
class vtkStringArray;
class vtkTypeUInt64Array;
class vtkAnnotationLink;

namespace GammaRay {

class vtkGraphAdapter : public QObject
{
    Q_OBJECT
public:
    struct Query
    {
        int column;
        int role;
    };

    explicit vtkGraphAdapter(QObject *parent = nullptr);
    ~vtkGraphAdapter() override;

    //
    // Qt side
    //
    void setVertexModel(QAbstractItemModel *model);
    void setVertexSelectionModel(QItemSelectionModel *model);
    void setVertexIdQuery(int column, int role);
    void setVertexLabelQuery(int column, int role);
    void setVertexClusterIdQuery(int column, int role);

    void setClusterModel(QAbstractItemModel *model);
    void setClusterSelectionModel(QItemSelectionModel *model);
    void setClusterIdQuery(int column, int role);
    void setClusterLabelQuery(int column, int role);

    void setEdgeModel(QAbstractItemModel *model);
    void setEdgeSelectionModel(QItemSelectionModel *model);
    void setEdgeIdQuery(int column, int role);
    void setEdgeLabelQuery(int column, int role);
    void setEdgeSourceIdQuery(int column, int role);
    void setEdgeTargetIdQuery(int column, int role);
    void setEdgeWeightQuery(int column, int role);

public slots:
    void recompute();

private slots:
    void vertexSelectionChanged(const QItemSelection &current, const QItemSelection &previous);
    void edgeSelectionChanged(const QItemSelection &current, const QItemSelection &previous);

public:
    //
    // VTK side
    //
    void setAnnotationLink(vtkAnnotationLink *link);
    vtkGraph *graph();

signals:
    void graphChanged();

private:
    vtkSmartPointer<vtkMutableDirectedGraph> m_graph;
    vtkSmartPointer<vtkAnnotationLink> m_annotationLink;
    void annotationChangedEvent();
    void updateAnnotation();

    QAbstractItemModel *m_vertexModel = nullptr;
    QItemSelectionModel *m_vertexSelectionModel = nullptr;
    quint64 vertexId(int index);
    vtkSmartPointer<vtkTypeUInt64Array> m_vertexIds;
    int m_vertexIdColumn = -1;
    int m_vertexIdRole = -1;
    std::string vertexLabel(int index);
    vtkSmartPointer<vtkStringArray> m_vertexLabels;
    int m_vertexLabelColumn = -1;
    int m_vertexLabelRole = -1;
    quint64 vertexClusterId(int index);
    vtkSmartPointer<vtkTypeUInt64Array> m_vertexClusterIds;
    int m_vertexClusterIdColumn = -1;
    int m_vertexClusterIdRole = -1;

    QAbstractItemModel *m_clusterModel = nullptr;
    QItemSelectionModel *m_clusterSelectionModel = nullptr;
    quint64 clusterId(int index);
    vtkSmartPointer<vtkTypeUInt64Array> m_clusterIds;
    int m_clusterIdColumn = -1;
    int m_clusterIdRole = -1;
    std::string clusterLabel(int index);
    vtkSmartPointer<vtkStringArray> m_clusterLabels;
    int m_clusterLabelColumn = -1;
    int m_clusterLabelRole = -1;

    QAbstractItemModel *m_edgeModel = nullptr;
    QItemSelectionModel *m_edgeSelectionModel = nullptr;
    quint64 edgeId(int index);
    vtkSmartPointer<vtkTypeUInt64Array> m_edgeIds;
    int m_edgeIdColumn = -1;
    int m_edgeIdRole = -1;
    std::string edgeLabel(int index);
    vtkSmartPointer<vtkStringArray> m_edgeLabels;
    int m_edgeLabelColumn = -1;
    int m_edgeLabelRole = -1;
    quint64 edgeSourceId(int index);
    int m_edgeSourceColumn = -1;
    int m_edgeSourceRole = -1;
    quint64 edgeTargetId(int index);
    int m_edgeTargetColumn = -1;
    int m_edgeTargetRole = -1;
    double edgeWeight(int index);
    vtkSmartPointer<vtkDoubleArray> m_edgeWeights;
    int m_edgeWeightColumn = -1;
    int m_edgeWeightRole = -1;
};

} // namespace GammaRay
#endif // VTKGRAPHADAPTER_H
