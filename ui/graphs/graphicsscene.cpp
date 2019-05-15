#include "graphicsscene.h"

#include "graphicsitem.h"

#include <QMap>
#include <QTimer>

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>

using namespace GammaRay::Graphs;
using ConnGraphPrivate =
    boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS>;

namespace GammaRay {
namespace Graphs {
class ConnGraph {

public:
    using VertexDescriptor = ConnGraphPrivate::vertex_descriptor;
    using EdgeDescriptor = ConnGraphPrivate::edge_descriptor;

    ConnGraph() {}

    inline VertexDescriptor addVertex() { return boost::add_vertex(m_graph); }

    inline EdgeDescriptor addEdge(VertexDescriptor from, VertexDescriptor to) {
        auto result = boost::add_edge(from, to, m_graph);
        assert(result.second);
        return result.first;
    }

    QMap<VertexDescriptor, GraphicsNodeItem *> vertexItem;
    QMap<GraphicsNodeItem *, VertexDescriptor> vertexId;

    QMap<EdgeDescriptor, GraphicsEdgeItem *> edgeItem;
    QMap<GraphicsNodeItem *, EdgeDescriptor> edgeId;

private:
    ConnGraphPrivate m_graph;
};
}
}

GraphicsScene::GraphicsScene(QObject *parent)
    : QGraphicsScene(parent), m_graph(new ConnGraph()) {
    auto vd = m_graph->addVertex();

    auto timer = new QTimer(this);
    timer->setInterval(500);
    connect(timer, &QTimer::timeout, this, [this]() {

    });
}

GraphicsScene::~GraphicsScene() {}

GraphicsNodeItem *GraphicsScene::addNode() {
    auto id = m_graph->addVertex();
    auto item = new GraphicsNodeItem();
    m_graph->vertexItem.insert(id, item);
    m_graph->vertexId.insert(item, id);
    addItem(item);
    return item;
}

GraphicsEdgeItem *GraphicsScene::addEdge(GraphicsNodeItem *from, GraphicsNodeItem *to)
{
    return {};
}
