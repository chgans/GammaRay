#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
class QAbstractItemModel;
QT_END_NAMESPACE

namespace GammaRay {

class ConnectivityRecordingInterface;

namespace Ui {
class RecordingWidget;
}

class RecordingWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RecordingWidget(QWidget *parent = nullptr);
    ~RecordingWidget();

    void setup(ConnectivityRecordingInterface *interface,
               QAbstractItemModel *model);

private:
    QScopedPointer<Ui::RecordingWidget> m_ui;
    ConnectivityRecordingInterface *m_interface = nullptr;
    QAbstractItemModel *m_model = nullptr;
};

} // namespace GammaRay
