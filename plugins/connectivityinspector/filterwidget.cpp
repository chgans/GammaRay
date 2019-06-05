#include "filterwidget.h"
#include "ui_filterwidget.h"

#include "filterinterface.h"

#include <ui/searchlinecontroller.h>

using namespace GammaRay;

FilterWidget::FilterWidget(QWidget *parent)
    : QWidget(parent), m_ui(new Ui::FilterWidget) {
    m_ui->setupUi(this);
}

FilterWidget::~FilterWidget() = default;

void FilterWidget::setup(FilterInterface *interface,
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
            &FilterInterface::recordAll);
    connect(m_ui->recordNoneButton, &QToolButton::clicked, m_interface,
            &FilterInterface::recordNone);
    connect(m_ui->showAllButton, &QToolButton::clicked, m_interface,
            &FilterInterface::showAll);
    connect(m_ui->showNonoeButton, &QToolButton::clicked, m_interface,
            &FilterInterface::showNone);
}
