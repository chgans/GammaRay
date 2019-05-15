#include "graphicsitem.h"

#include <QPainter>

using namespace GammaRay::Graphs;

GraphicsItem::GraphicsItem(GraphicsItem *parent) : QGraphicsItem(parent) {}

GraphicsItem::~GraphicsItem() {}

GraphicsEdgeItem::GraphicsEdgeItem(GraphicsItem *parent)
    : GraphicsItem(parent) {}

GraphicsEdgeItem::~GraphicsEdgeItem() {}

GraphicsNodeItem::GraphicsNodeItem(GraphicsItem *parent)
    : GraphicsItem(parent) {
    m_path.addEllipse(QRectF(-5, -5, 5, 5));
}

GraphicsNodeItem::~GraphicsNodeItem() {}


QRectF GammaRay::Graphs::GraphicsNodeItem::boundingRect() const {
    return m_path.boundingRect();
}

void GammaRay::Graphs::GraphicsNodeItem::paint(
    QPainter *painter, const QStyleOptionGraphicsItem *option,
    QWidget *widget) {

    painter->setPen(QPen(Qt::blue, 1.0));
    painter->setBrush(Qt::darkBlue);
    painter->drawPath(m_path);
}

QRectF GammaRay::Graphs::GraphicsEdgeItem::boundingRect() const {
    return m_path.boundingRect();
}

void GammaRay::Graphs::GraphicsEdgeItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{

}
