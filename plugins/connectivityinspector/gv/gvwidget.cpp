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

#include "gvwidget.h"

#include "connectionmodel.h"
#include "gvgraphicsview.h"
#include "gvrenderer.h"

#include "common/objectid.h"

#include <graphviz-qt/graphviz-qt.h>

#include <graphviz-qt/edge.h>
#include <graphviz-qt/graph.h>
#include <graphviz-qt/node.h>

#include <QAbstractTableModel>
#include <QElapsedTimer>
#include <QGraphicsScene>
#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>

#include <iostream>

using namespace GammaRay;
using namespace graphviz;

GvWidget::GvWidget(QWidget *parent)
    : QWidget(parent)
    , m_renderTimer(new QTimer(this))
    , m_statusBar(new QLabel(this))
    , m_scene(new QGraphicsScene(this))
    , m_view(new GvGraphicsView(m_scene, this))
    , m_gvContext(new GraphViz())
    , m_gvRenderer(new GvRenderer(m_scene))
{
    m_gvContext->open();
    auto layout = new QVBoxLayout(this);
    layout->addWidget(m_view);
    connect(m_renderTimer, &QTimer::timeout, this, &GvWidget::renderGraph);
    layout->addWidget(m_statusBar);
    m_statusBar->setText("Loading...");
}

GvWidget::~GvWidget()
{
    m_gvContext->close();
    delete m_gvContext;
}

void GvWidget::setModel(QAbstractItemModel *model) {
    if (m_model) {
        m_scene->clear();
        m_renderTimer->stop();
        m_model->disconnect(this);
    }

    m_model = model;

    if (m_model) {
        renderGraph();
        m_renderTimer->setSingleShot(true);
        // m_renderTimer->start(std::chrono::milliseconds(100));
    }
}

void GvWidget::setLayoutAlgorithm(AlgorithmType algo) {
    if (m_currentAlgorithm == algo)
        return;
    m_currentAlgorithm = algo;
    if (!m_model)
        return;
    renderGraph();
}

void GvWidget::setNodeSeparation(double value) {
    if (qFuzzyCompare(m_nodeSeparation, value))
        return;
    m_nodeSeparation = value;
    if (!m_model)
        return;
    renderGraph();
}

void GvWidget::setRankSeparation(double value) {
    if (qFuzzyCompare(m_rankSeparation, value))
        return;
    m_rankSeparation = value;
    if (!m_model)
        return;
    renderGraph();
}

void GvWidget::processRow(int row) {
    QModelIndex index;
    Graph senderGraph;
    Graph receiverGraph;
    Node senderNode;
    Node receiverNode;
    //    {
    //        constexpr int ID = ConnectionModel::ThreadIdRole;
    //        index = m_model->index(row, ConnectionModel::SenderColumn);
    //        auto senderId = index.data(ID).value<ObjectId>().id();
    //        auto senderName = QStringLiteral("cluster_%1").arg(senderId);
    //        senderGraph = m_gvGraph.addSubgraph(senderName);
    //        index = m_model->index(row, ConnectionModel::ReceiverColumn);
    //        auto receiverId = index.data(ID).value<ObjectId>().id();
    //        auto receiverName = QStringLiteral("cluster_%1").arg(receiverId);
    //        receiverGraph = m_gvGraph.addSubgraph(receiverName);
    //    }
    //    {
    //        constexpr int ID = ConnectionModel::ObjectIdRole;
    //        index = m_model->index(row, ConnectionModel::SenderColumn);
    //        auto senderId = index.data(ID).value<ObjectId>().id();
    //        auto senderName = index.data().toString();
    //        senderNode = senderGraph.addNode(senderName);
    //        index = m_model->index(row, ConnectionModel::ReceiverColumn);
    //        auto receiverId = index.data(ID).value<ObjectId>().id();
    //        auto receiverName = index.data().toString();
    //        receiverNode = receiverGraph.addNode(receiverName);
    //    }
    //    m_gvGraph.addEdge(senderNode, receiverNode);
}

#define MISSING_GVQT_FIXES
void GvWidget::renderGraph() {
    QElapsedTimer timer;
    timer.start();

    m_scene->clear();
    m_renderingTime = timer.restart();

    // FIXME
#ifndef MISSING_GVQT_FIXES
    if (m_gvGraph.isValid())
        m_gvContext->discardGraph(m_gvGraph);
#endif
    m_gvGraph = m_gvContext->newGraph("root", DirectedGraph);
    m_gvGraph.setGraphAttribute("concentrate", "true");
    m_gvGraph.setGraphAttribute("ranksep", QString::number(m_rankSeparation));
    m_gvGraph.setGraphAttribute("nodesep", QString::number(m_nodeSeparation));
    m_gvGraph.setNodeAttribute("label", "");
    m_gvGraph.setNodeAttribute("shape", "circle");
    //m_gvGraph.setEdgeAttribute("splines", "spline");
    m_gvGraph.setEdgeAttribute("arrowhead", "none");
    for (int row = 0; row < m_model->rowCount(); ++row)
        processRow(row);
    m_creatingTime = timer.restart();

#ifndef MISSING_GVQT_FIXES
    auto result = m_gvContext->render(m_gvRenderer, m_gvGraph, m_currentAlgorithm);
    m_layoutingTime = result.first;
    m_renderingTime += result.second;
#else
    m_gvContext->render(m_gvRenderer, m_gvGraph, m_currentAlgorithm);
#endif
    m_clusterCount = m_gvGraph.subGraphCount();
    m_nodeCount = m_gvGraph.nodeCount();
#ifndef MISSING_GVQT_FIXES
    m_edgeCount = m_gvGraph.edgeCount();
#endif
    updateStatusBar();
    QTimer::singleShot(0, this, [this]() {
        m_view->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
    });
}

void GvWidget::updateStatusBar()
{
    auto msg = QStringLiteral(
                   "Graph: {Clusters: %1, Nodes: %2, Edges: %3}, Processing: {create: %4ms, "
                   "layout: %5ms, render: %6ms}, Scene: {size: %7\"x%8\", items:%9}")
                   .arg(m_clusterCount)
                   .arg(m_nodeCount)
                   .arg(m_edgeCount)
                   .arg(m_creatingTime)
                   .arg(m_layoutingTime)
                   .arg(m_renderingTime)
                   .arg(m_scene->width())
                   .arg(m_scene->height())
                   .arg(m_scene->items().count());
    m_statusBar->setText(msg);
}
