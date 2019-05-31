#include "gvwidget.h"

#include "gvgraphicsview.h"
#include "gvrenderer.h"
#include "objectvisualizermodel.h"

#include "common/objectid.h"

#include <graphviz-qt/graphviz-qt.h>

#include <graphviz-qt/edge.h>
#include <graphviz-qt/graph.h>
#include <graphviz-qt/node.h>

#include <QAbstractTableModel>
#include <QGraphicsScene>
#include <QTimer>
#include <QVBoxLayout>

#include <iostream>

using namespace GammaRay;
using namespace graphviz;

GvWidget::GvWidget(QWidget *parent)
    : QWidget(parent), m_scene(new QGraphicsScene(this)),
      m_view(new GvGraphicsView(m_scene, this)), m_gvContext(new GraphViz()),
      m_gvRenderer(new GvRenderer(m_scene)) {
    m_gvContext->open();
    auto layout = new QVBoxLayout(this);
    layout->addWidget(m_view);
}

GvWidget::~GvWidget() {
    m_gvContext->close();
    delete m_gvContext;
}

void GvWidget::setModel(QAbstractItemModel *model) {
    m_scene->clear();
    if (m_model)
        m_model->disconnect(this);
    m_model = model;
    connect(m_model, &QAbstractItemModel::rowsInserted, this,
            &GvWidget::renderGraph);
    connect(m_model, &QAbstractItemModel::rowsRemoved, this,
            &GvWidget::renderGraph);
    connect(m_model, &QAbstractItemModel::dataChanged, this,
            &GvWidget::renderGraph);
    renderGraph();
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
    {
        constexpr int ID = ObjectVisualizerModel::ThreadIdRole;
        index = m_model->index(row, ObjectVisualizerModel::SenderColumn);
        auto senderId = index.data(ID).value<ObjectId>().id();
        auto senderName = QStringLiteral("cluster_%1").arg(senderId);
        senderGraph = m_gvGraph.addSubgraph(senderName);
        index = m_model->index(row, ObjectVisualizerModel::ReceiverColumn);
        auto receiverId = index.data(ID).value<ObjectId>().id();
        auto receiverName = QStringLiteral("cluster_%1").arg(receiverId);
        receiverGraph = m_gvGraph.addSubgraph(receiverName);
    }
    {
        constexpr int ID = ObjectVisualizerModel::ObjectIdRole;
        index = m_model->index(row, ObjectVisualizerModel::SenderColumn);
        auto senderId = index.data(ID).value<ObjectId>().id();
        auto senderName = QString::number(senderId);
        senderNode = senderGraph.addNode(senderName);
        index = m_model->index(row, ObjectVisualizerModel::ReceiverColumn);
        auto receiverId = index.data(ID).value<ObjectId>().id();
        auto receiverName = QString::number(receiverId);
        receiverNode = receiverGraph.addNode(receiverName);
        std::cout << "Adding edge " << senderId << " -> " << receiverId
                  << std::endl;
    }
    m_gvGraph.addEdge(senderNode, receiverNode);
}

void GvWidget::renderGraph() {
    m_scene->clear();
    m_gvGraph = m_gvContext->newGraph("root", DirectedGraph);
    // m_gvGraph.setGraphAttribute("ranksep", "1.5");
    m_gvGraph.setGraphAttribute("ranksep", QString::number(m_rankSeparation));
    m_gvGraph.setGraphAttribute("nodesep", QString::number(m_nodeSeparation));
    m_gvGraph.setNodeAttribute("label", "");
    m_gvGraph.setNodeAttribute("shape", "point");
    // m_gvGraph.setEdgeAttribute("splines", "spline");
    for (int row = 0; row < m_model->rowCount(); ++row)
        processRow(row);
    m_gvContext->render(m_gvRenderer, m_gvGraph, m_currentAlgorithm);
    QTimer::singleShot(0, this, [this]() {
        m_view->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
    });
}
