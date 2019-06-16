/*
  This file is part of GammaRay, the Qt application inspection and
  manipulation tool.

  Copyright (C) 2019 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Christian Gagneraud <chgans@gmail.com>

  Licensees holding valid commercial KDAB GammaRay licenses may use this file in
  accordance with GammaRay Commercial License Agreement provided with the Software.

  Contact info@kdab.com if any conditions of this licensing are not clear to you.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "vtkcontainer.h"

#include <QLabel>
#include <QVBoxLayout>

#include "vtkwidget.h"
#include "vtkpanel.h"

using namespace GammaRay;

VtkContainer::VtkContainer(QWidget *parent) : QWidget(parent) {
    QVBoxLayout *vbox = new QVBoxLayout(this);

    m_vtkWidget = new VtkWidget(this);
    m_vtkPanel = new VtkPanel(this);
    m_statusLabel = new QLabel(this);
    vbox->addWidget(m_vtkPanel);
    vbox->addWidget(m_vtkWidget);
    vbox->addWidget(m_statusLabel);

    m_vtkPanel->setLayoutStrategy(Vtk::LayoutStrategy::SpanTree);
    m_vtkPanel->setStereoMode(Vtk::StereoMode::Off);
    m_vtkPanel->setTheme(Vtk::ThemeType::Neon);
    m_vtkPanel->setShowNodeLabel(true);
    m_vtkPanel->setShowEdgeLabel(false);
    m_vtkPanel->setShowEdgeArrow(false);

    m_vtkWidget->setLayoutStrategy(m_vtkPanel->layoutStrategy());
    m_vtkWidget->setStereoMode(m_vtkPanel->stereoMode());
    m_vtkWidget->setThemeType(m_vtkPanel->themeType());
    m_vtkWidget->setShowNodeLabel(m_vtkPanel->showNodeLabel());
    m_vtkWidget->setShowEdgeLabel(m_vtkPanel->showEdgeLabel());
    m_vtkWidget->setShowEdgeArrow(m_vtkPanel->showEdgeArrow());

    connect(m_vtkPanel,
            &VtkPanel::layoutStrategyChanged,
            m_vtkWidget,
            &VtkWidget::setLayoutStrategy);
    connect(m_vtkPanel, &VtkPanel::stereoModeChanged, m_vtkWidget, &VtkWidget::setStereoMode);
    connect(m_vtkPanel, &VtkPanel::themeChanged, m_vtkWidget, &VtkWidget::setThemeType);
    connect(m_vtkPanel, &VtkPanel::showNodeLabelChanged, m_vtkWidget, &VtkWidget::setShowNodeLabel);
    connect(m_vtkPanel, &VtkPanel::showEdgeLabelChanged, m_vtkWidget, &VtkWidget::setShowEdgeLabel);
    connect(m_vtkPanel, &VtkPanel::showEdgeArrowChanged, m_vtkWidget, &VtkWidget::setShowEdgeArrow);

    connect(m_vtkWidget, &VtkWidget::statusChanged, m_statusLabel, &QLabel::setText);
}

VtkContainer::~VtkContainer() {}

vtkGraphAdapter *VtkContainer::graphAdapater()
{
    return m_vtkWidget->graphAdapter();
}
