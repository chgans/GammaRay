#include "gvpanel.h"

#include "gvwidget.h"

#include <graphviz-qt/graphviz-qt.h>

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>

using namespace GammaRay;
using namespace graphviz;

GvPanel::GvPanel(GvWidget *gvWidget, QWidget *parent)
    : QToolBar(parent), m_gvWidget(gvWidget) {

    addWidget(new QLabel(tr("Layout:")));
    m_layoutComboBox = new QComboBox;
    m_layoutComboBox->addItem("Circular",
                              QVariant::fromValue<>(CircularLayout));
    m_layoutComboBox->addItem("Cluster", QVariant::fromValue<>(ClusterLayout));
    m_layoutComboBox->addItem("Hierarchical",
                              QVariant::fromValue<>(HierarchicalLayout));
    m_layoutComboBox->addItem("Energy", QVariant::fromValue<>(EnergyLayout));
    m_layoutComboBox->addItem("Force", QVariant::fromValue<>(ForceLayout));
    m_layoutComboBox->addItem("Force (large)",
                              QVariant::fromValue<>(LargeForceLayout));
    m_layoutComboBox->addItem("Tree", QVariant::fromValue<>(TreeLayout));
    m_layoutComboBox->addItem("Radial", QVariant::fromValue<>(RadialLayout));
    m_layoutComboBox->setCurrentIndex(1);
    connect(m_layoutComboBox, &QComboBox::currentTextChanged, this, [this]() {
        auto algo = m_layoutComboBox->currentData().value<LayoutType>();
        m_gvWidget->setLayoutAlgorithm(algo);
    });
    m_gvWidget->setLayoutAlgorithm(m_layoutComboBox->currentData().value<LayoutType>());
    addWidget(m_layoutComboBox);

    addWidget(new QLabel(" Rank sep."));
    m_rankSepSpinBox = new QDoubleSpinBox();
    m_rankSepSpinBox->setValue(0.2);
    m_rankSepSpinBox->setSingleStep(0.1);
    connect(m_rankSepSpinBox,
            QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            [this](double value) { m_gvWidget->setRankSeparation(value); });
    m_gvWidget->setRankSeparation(m_rankSepSpinBox->value());
    addWidget(m_rankSepSpinBox);

    addWidget(new QLabel(" Node sep."));
    m_nodeSepSpinBox = new QDoubleSpinBox();
    m_nodeSepSpinBox->setValue(0.2);
    m_nodeSepSpinBox->setSingleStep(0.1);
    connect(m_nodeSepSpinBox,
            QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            [this](double value) { m_gvWidget->setNodeSeparation(value); });
    m_gvWidget->setNodeSeparation(m_nodeSepSpinBox->value());
    addWidget(m_nodeSepSpinBox);
}
