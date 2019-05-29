#pragma once

#include <QWidget>

class QAbstractItemModel;

namespace GammaRay {
class GvPanel;
class GvWidget;

class GvContainer : public QWidget
{
    Q_OBJECT
public:
    explicit GvContainer(QWidget *parent = nullptr);
    ~GvContainer() override;

    void setModel(QAbstractItemModel *model);
signals:

public slots:

private:
    GvPanel *m_panel;
    GvWidget *m_widget;
};

} // namespace GammaRay
