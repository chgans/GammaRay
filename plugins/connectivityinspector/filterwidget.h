#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
class QAbstractItemModel;
QT_END_NAMESPACE

namespace GammaRay {

class FilterInterface;

namespace Ui {
class FilterWidget;
}

class FilterWidget : public QWidget {
    Q_OBJECT

public:
    explicit FilterWidget(QWidget *parent = nullptr);
    ~FilterWidget();

    void setup(FilterInterface *interface, QAbstractItemModel *model);

private:
    QScopedPointer<Ui::FilterWidget> m_ui;
    FilterInterface *m_interface = nullptr;
    QAbstractItemModel *m_model = nullptr;
};

} // namespace GammaRay
