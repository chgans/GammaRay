#pragma once

#include <QScopedPointer>
#include <QWidget>

namespace GammaRay {
class AcquisitionInterface;

namespace Ui {
class AcquisitionWidget;
}

class AcquisitionWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AcquisitionWidget(QWidget *parent = nullptr);
    ~AcquisitionWidget();

    void setAcquisitionInterface(AcquisitionInterface *interface);

private:
    QScopedPointer<Ui::AcquisitionWidget> m_ui;
    AcquisitionInterface *m_interface = nullptr;
};

} // namespace GammaRay
