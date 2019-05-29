#pragma once

#include <QToolBar>

class QComboBox;
class QDoubleSpinBox;

namespace GammaRay {
class GvWidget;

class GvPanel : public QToolBar {
    Q_OBJECT
public:
    explicit GvPanel(GvWidget *gvWidget, QWidget *parent = nullptr);

signals:

public slots:

private:
    GvWidget *m_gvWidget;
    QComboBox *m_layoutComboBox;
    QDoubleSpinBox *m_nodeSepSpinBox;
    QDoubleSpinBox *m_rankSepSpinBox;
};

} // namespace GammaRay
