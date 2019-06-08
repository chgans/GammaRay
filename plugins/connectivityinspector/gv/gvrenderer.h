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

#ifndef GAMMARAY_CONNECTIVITYINSPECTOR_GVRENDERER_H
#define GAMMARAY_CONNECTIVITYINSPECTOR_GVRENDERER_H

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

#endif // GAMMARAY_CONNECTIVITYINSPECTOR_GVRENDERER_H
