#include "gvrenderer.h"

#include "graphviz-qt/edge.h"
#include "graphviz-qt/graph.h"
#include "graphviz-qt/node.h"

#include <QtCore/QDebug>
#include <QtCore/QRectF>
#include <QtGui/QFont>
#include <QtGui/QFontMetricsF>
#include <QtGui/QPainterPath>
#include <QtWidgets/QGraphicsItemGroup>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsSimpleTextItem>

using namespace GammaRay;

// TODO: Use custom and efficient graphics items

GvRenderer::GvRenderer(QGraphicsScene *scene)
    : graphviz::IRender(), m_scene(scene) {}

void GvRenderer::beginJob() {
    m_group = new QGraphicsPathItem({});
    m_group->setData(0, "Job");
    m_group->setData(1, "0");
}

void GvRenderer::endJob() {
    const auto box = deviceBoundingBox();
    m_scene->setBackgroundBrush(QColor(0, 43, 54));
    m_scene->addRect(box, Qt::NoPen, QColor(7, 54, 66));
    m_scene->setSceneRect(box);
    m_scene->addItem(m_group);
    m_group = nullptr;
}

void GvRenderer::beginGraph(graphviz::Graph graph) {
    pushGroup("Graph", graph.name());
}

void GvRenderer::endGraph() { popGroup(); }

void GvRenderer::beginCluster(graphviz::Graph graph) {
    // FIXME: begin/ednCluster works like a tag on a graph
    Q_UNUSED(graph);
}

void GvRenderer::endCluster() {}

void GvRenderer::beginNode(graphviz::Node node) {
    pushGroup("Node", node.name());
    m_pen = QPen(QColor(101, 123, 131), 0.0);
}

void GvRenderer::endNode() { popGroup(); }

void GvRenderer::beginEdge(graphviz::Edge edge) {
    pushGroup("Edge", edge.name());
    m_pen = QPen(QColor(101, 123, 131), 0.0);
}

void GvRenderer::endEdge() { popGroup(); }

void GvRenderer::addText(const QString &text, const QPointF &pos,
                         const QFont &font, const QBrush &brush,
                         Qt::Alignment alignment) {
    static int count = 0;
    double factor = 0.0;
    if (alignment & Qt::AlignHCenter)
        factor = -0.5;
    else if (alignment & Qt::AlignHCenter)
        factor = -1.0;
    auto item = new QGraphicsSimpleTextItem(text, m_group);
    item->setFont(font);
    const auto bb = item->boundingRect();
    const double xOffset = factor * bb.width();
    item->setPos(pos.x() + xOffset, pos.y() - bb.height());
    item->setBrush(m_pen.color());
    item->setData(0, QStringLiteral("Text#%1").arg(count++));
}

void GvRenderer::addEllipse(const QRectF &rect, const QPen &pen,
                            const QBrush &brush) {
    static int count = 0;
    auto item = new QGraphicsEllipseItem(rect, m_group);
    item->setPen(m_pen);
    // item->setBrush(brush);
    item->setData(0, QStringLiteral("Ellipse#%1").arg(count++));
}

void GvRenderer::addPolygon(const QVector<QPointF> &points, const QPen &pen,
                            const QBrush &brush) {
    static int count = 0;
    QPainterPath path;
    path.moveTo(points.at(0));
    int i = 1;
    while (i < points.size()) {
        path.lineTo(points.at(i));
        i++;
    }
    path.lineTo(points.at(0));
    auto item = new QGraphicsPathItem(path, m_group);
    item->setPen(m_pen);
    // item->setBrush(brush);
    item->setData(0, QStringLiteral("Polygon#%1").arg(count++));
}

void GvRenderer::addBezierCurve(const QVector<QPointF> &points,
                                const QPen &pen) {
    static int count = 0;
    QPainterPath path;
    path.moveTo(points.at(0));
    int i = 1;
    while (i < points.size()) {
        path.cubicTo(points.at(i), points.at(i + 1), points.at(i + 2));
        i += 3;
    }
    auto item = new QGraphicsPathItem(path, m_group);
    item->setPen(m_pen);
    item->setData(0, QStringLiteral("Curve#%1").arg(count++));
}

void GvRenderer::addPolyline(const QVector<QPointF> &points, const QPen &pen) {
    static int count = 0;
    QPainterPath path;
    path.moveTo(points.at(0));
    int i = 1;
    while (i < points.size()) {
        path.lineTo(points.at(i));
        i++;
    }
    auto item = new QGraphicsPathItem(path, m_group);
    item->setPen(m_pen);
    item->setData(0, QStringLiteral("Line#%1").arg(count++));
}

void GvRenderer::pushGroup(const QString &type, const QString &name) {
    m_group = new QGraphicsPathItem({}, m_group);
    m_group->setData(0, type);
    m_group->setData(1, name);
}

void GvRenderer::popGroup() { m_group = m_group->parentItem(); }
