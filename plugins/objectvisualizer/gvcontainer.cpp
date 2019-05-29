#include "gvcontainer.h"

#include "gvpanel.h"
#include "gvwidget.h"

#include <QVBoxLayout>

using namespace GammaRay;

GvContainer::GvContainer(QWidget *parent) : QWidget(parent) {
    QVBoxLayout *vbox = new QVBoxLayout(this);
    m_widget = new GvWidget(this);
    m_panel = new GvPanel(m_widget, this);
    vbox->addWidget(m_panel);
    vbox->addWidget(m_widget);
}

void GvContainer::setModel(QAbstractItemModel *model) {
    m_widget->setModel(model);
}

GvContainer::~GvContainer() = default;
