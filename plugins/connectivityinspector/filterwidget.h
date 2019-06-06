#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
class QAbstractItemModel;
QT_END_NAMESPACE

namespace GammaRay {

class DiscriminatorInterface;

namespace Ui {
class FilterWidget;
}

class FilterWidget : public QWidget {
    Q_OBJECT

public:
    explicit FilterWidget(QWidget *parent = nullptr);
    ~FilterWidget();

    void setup(DiscriminatorInterface *interface, QAbstractItemModel *model);

private:
    QScopedPointer<Ui::FilterWidget> m_ui;
    DiscriminatorInterface *m_interface = nullptr;
    QAbstractItemModel *m_model = nullptr;
};

} // namespace GammaRay
