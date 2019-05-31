#pragma once

#include <graphviz-qt/renderer.h>

#include <QPen>

class QGraphicsScene;
class QGraphicsItem;

namespace GammaRay {

class GvRenderer : public graphviz::IRender
{
public:
    GvRenderer(QGraphicsScene *scene);

    // IRender interface
public:
    void beginJob() override;
    void endJob() override;
    void beginGraph(graphviz::Graph graph) override;
    void endGraph() override;
    void beginCluster(graphviz::Graph graph) override;
    void endCluster() override;
    void beginNode(graphviz::Node node) override;
    void endNode() override;
    void beginEdge(graphviz::Edge edge) override;
    void endEdge() override;
    void addText(const QString &text, const QPointF &pos, const QFont &font,
                 const QBrush &brush, Qt::Alignment alignment) override;
    void addEllipse(const QRectF &rect, const QPen &pen,
                    const QBrush &brush) override;
    void addPolygon(const QVector<QPointF> &points, const QPen &pen,
                    const QBrush &brush) override;
    void addBezierCurve(const QVector<QPointF> &points,
                        const QPen &pen) override;
    void addPolyline(const QVector<QPointF> &points, const QPen &pen) override;

private:
    QGraphicsScene *m_scene = nullptr;
    QGraphicsItem *m_group = nullptr;
    void pushGroup(const QString &type, const QString &name);
    void popGroup();
    QPen m_pen = Qt::NoPen;
    QBrush m_brush = Qt::NoBrush;
};

} // namespace GammaRay
