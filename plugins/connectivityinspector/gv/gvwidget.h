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

#ifndef GAMMARAY_CONNECTIVITYINSPECTOR_GVWIDGET_H
#define GAMMARAY_CONNECTIVITYINSPECTOR_GVWIDGET_H

#include <QWidget>

#include <graphviz-qt/graph.h>

class QGraphicsScene;
class QAbstractItemModel;
class QLabel;

namespace graphviz {
class GraphViz;
}
namespace GammaRay {
class GvGraphicsView;
class GvRenderer;

using AlgorithmType = graphviz::LayoutType;

class GvWidget : public QWidget
{
    Q_OBJECT
public:
    explicit GvWidget(QWidget *parent = nullptr);
    ~GvWidget() override;

    // TODO: 4 list/table models?
    //  MetaObject, Object, Thread and Connection
    void setModel(QAbstractItemModel *model);

public slots:
    void setLayoutAlgorithm(AlgorithmType algo);
    void setNodeSeparation(double value);
    void setRankSeparation(double value);

private:
    void processRow(int row);
    void renderGraph();
    void updateStatusBar();
    QTimer *m_renderTimer;
    QLabel *m_statusBar;
    QGraphicsScene *m_scene = nullptr;
    GvGraphicsView *m_view = nullptr;
    QAbstractItemModel *m_model = nullptr;
    graphviz::GraphViz *m_gvContext = nullptr;
    GvRenderer *m_gvRenderer = nullptr;
    graphviz::Graph m_gvGraph;
    AlgorithmType m_currentAlgorithm;
    double m_rankSeparation = 0.1;
    double m_nodeSeparation = 0.1;
    int m_clusterCount;
    int m_nodeCount;
    int m_edgeCount;
    qint64 m_creatingTime;
    qint64 m_layoutingTime;
    qint64 m_renderingTime;
};

} // namespace GammaRay

#endif // GAMMARAY_CONNECTIVITYINSPECTOR_GVWIDGET_H
