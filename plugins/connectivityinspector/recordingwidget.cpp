#include "recordingwidget.h"
#include "ui_recordingwidget.h"

#include "recordinginterface.h"

#include <ui/searchlinecontroller.h>

using namespace GammaRay;

RecordingWidget::RecordingWidget(QWidget *parent)
    : QWidget(parent), m_ui(new Ui::RecordingWidget) {
    m_ui->setupUi(this);
}

RecordingWidget::~RecordingWidget() = default;

void RecordingWidget::setup(ConnectivityRecordingInterface *interface,
                            QAbstractItemModel *model) {
    Q_ASSERT(m_interface == nullptr);
    Q_ASSERT(m_model == nullptr);
    Q_ASSERT(interface != nullptr);
    Q_ASSERT(model != nullptr);

    m_interface = interface;
    m_model = model;

    auto view = m_ui->treeView;
    view->header()->setObjectName(
        QStringLiteral("%1ViewHeader").arg(m_interface->name()));
    new SearchLineController(m_ui->searchLine, m_model);
    view->setDeferredResizeMode(0, QHeaderView::ResizeToContents);
    view->setModel(m_model);
    view->setSortingEnabled(true);
    connect(m_ui->recordAllButton, &QToolButton::clicked, m_interface,
            &ConnectivityRecordingInterface::recordAll);
    connect(m_ui->recordNoneButton, &QToolButton::clicked, m_interface,
            &ConnectivityRecordingInterface::recordNone);
    connect(m_ui->showAllButton, &QToolButton::clicked, m_interface,
            &ConnectivityRecordingInterface::showAll);
    connect(m_ui->showNonoeButton, &QToolButton::clicked, m_interface,
            &ConnectivityRecordingInterface::showNone);
}
