#pragma once

#include <QGraphicsView>

namespace GammaRay {

class GvGraphicsView : public QGraphicsView {
public:
    GvGraphicsView(QGraphicsScene *scene, QWidget *parent = nullptr);
};
} // namespace GammaRay
