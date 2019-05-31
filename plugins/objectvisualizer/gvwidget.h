#pragma once

#include <QWidget>

#include <graphviz-qt/graph.h>

class QGraphicsScene;
class QAbstractItemModel;

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
    QGraphicsScene *m_scene = nullptr;
    GvGraphicsView *m_view = nullptr;
    QAbstractItemModel *m_model = nullptr;
    graphviz::GraphViz *m_gvContext = nullptr;
    GvRenderer *m_gvRenderer = nullptr;
    graphviz::Graph m_gvGraph;
    AlgorithmType m_currentAlgorithm;
    double m_rankSeparation = 0.1;
    double m_nodeSeparation = 0.1;
};

} // namespace GammaRay
