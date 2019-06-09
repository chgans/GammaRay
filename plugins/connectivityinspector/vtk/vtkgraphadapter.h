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
QT_END_NAMESPACE

class vtkGraph;
class vtkMutableDirectedGraph;
class vtkStringArray;
class vtkTypeUInt64Array;

namespace GammaRay {

class vtkGraphAdapter : public QObject
{
     Q_OBJECT
public:
    explicit vtkGraphAdapter(QObject *parent = nullptr);
    ~vtkGraphAdapter() override;

    void setVertexModel(const QAbstractItemModel *model);
    void setVertexIdQuery(int column, int role);
    void setVertexLabelQuery(int column, int role);
    void setEdgeModel(const QAbstractItemModel *model);
    void setEdgeIdQuery(int column, int role);
    void setEdgeLabelQuery(int column, int role);
    void setEdgeSourceIdQuery(int column, int role);
    void setEdgeTargetIdQuery(int column, int role);

    void setup();

    vtkGraph *graph();

signals:

public slots:

private:
    const QAbstractItemModel *m_vertexModel = nullptr;
    vtkStringArray *m_vertexLabels = nullptr;
    vtkTypeUInt64Array *m_vertexIds = nullptr;
    const QAbstractItemModel *m_edgeModel = nullptr;
    vtkStringArray *m_edgeLabels = nullptr;
    vtkTypeUInt64Array *m_edgeIds = nullptr;
    vtkSmartPointer<vtkMutableDirectedGraph> m_graph;
    int m_vertexIdColumn = 0;
    int m_vertexIdRole = 0;
    int m_vertexLabelColumn = 0;
    int m_vertexLabelRole = 0;
    int m_edgeIdColumn = 0;
    int m_edgeIdRole = 0;
    int m_edgeLabelColumn = 0;
    int m_edgeLabelRole = 0;
    int m_edgeSourceColumn = 0;
    int m_edgeSourceRole = 0;
    int m_edgeTargetColumn = 0;
    int m_edgeTargetRole = 0;

    quint64 vertexId(int index);
    std::string vertexLabel(int index);
    quint64 edgeId(int index);
    std::string edgeLabel(int index);
    quint64 edgeSourceId(int index);
    quint64 edgeTargetId(int index);
};

} // namespace GammaRay
#endif // VTKGRAPHADAPTER_H
