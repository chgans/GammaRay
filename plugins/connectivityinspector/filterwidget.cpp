#include "filterwidget.h"
#include "ui_filterwidget.h"

#include "discriminatorinterface.h"

#include <ui/searchlinecontroller.h>

using namespace GammaRay;

FilterWidget::FilterWidget(QWidget *parent)
    : QWidget(parent), m_ui(new Ui::FilterWidget) {
    m_ui->setupUi(this);
}

FilterWidget::~FilterWidget() = default;

void FilterWidget::setup(DiscriminatorInterface *interface,
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
    connect(m_ui->enableFilteringCheckBox,
            &QCheckBox::toggled,
            m_interface,
            &DiscriminatorInterface::setEnabled);
    connect(m_ui->enableAllButton,
            &QToolButton::clicked,
            m_interface,
            &DiscriminatorInterface::discriminateAll);
    connect(m_ui->disableAllButton,
            &QToolButton::clicked,
            m_interface,
            &DiscriminatorInterface::discriminateNone);
    connect(m_ui->filterAllButton,
            &QToolButton::clicked,
            m_interface,
            &DiscriminatorInterface::filterAll);
    connect(m_ui->passAllButton,
            &QToolButton::clicked,
            m_interface,
            &DiscriminatorInterface::filterNone);
}
