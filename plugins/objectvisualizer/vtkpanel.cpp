/*
  This file is part of GammaRay, the Qt application inspection and
  manipulation tool.

  Copyright (C) 2010-2019 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Kevin Funk <kevin.funk@kdab.com>

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

#include "vtkpanel.h"
#include "vtkwidget.h"

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>

using namespace GammaRay;

namespace {} // namespace

VtkPanel::VtkPanel(QWidget *parent)
    : QToolBar(parent)
{
    addWidget(new QLabel(tr("Layout: ")));

    createLayoutStrategyComboBox();
    connect(m_layoutStrategyComboBox, &QComboBox::currentTextChanged, this, [this]() {
        layoutStrategyChanged(layoutStrategy());
    });
    m_layoutStrategyComboBox->setCurrentIndex(0);
    addWidget(m_layoutStrategyComboBox);

    addWidget(new QLabel(tr(" Stereo: ")));
    createStereoModeComboBox();
    connect(m_stereoModeComboBox, &QComboBox::currentTextChanged, this, [this]() {
        stereoModeChanged(stereoMode());
    });
    m_stereoModeComboBox->setCurrentIndex(0);
    addWidget(m_stereoModeComboBox);

    addWidget(new QLabel(tr(" Theme: ")));
    createThemeComboBox();
    connect(m_themeTypeComboBox, &QComboBox::currentTextChanged, this, [this]() {
        themeChanged(themeType());
    });
    m_themeTypeComboBox->setCurrentIndex(0);
    addWidget(m_themeTypeComboBox);

    addWidget(new QLabel(tr(" Node: ")));
    m_showNodeLabelCheckBox = new QCheckBox(this);
    connect(m_showNodeLabelCheckBox, &QCheckBox::clicked, this, &VtkPanel::showNodeLabelChanged);
    addWidget(m_showNodeLabelCheckBox);

    addWidget(new QLabel(tr(" Edges: ")));
    m_showEdgeLabelCheckBox = new QCheckBox(this);
    connect(m_showEdgeLabelCheckBox, &QCheckBox::clicked, this, &VtkPanel::showEdgeLabelChanged);
    addWidget(m_showEdgeLabelCheckBox);

    addWidget(new QLabel(tr(" Arrows: ")));
    m_showEdgeArrowCheckBox = new QCheckBox(this);
    connect(m_showEdgeArrowCheckBox, &QCheckBox::clicked, this, &VtkPanel::showEdgeArrowChanged);
    addWidget(m_showEdgeArrowCheckBox);
}

VtkPanel::~VtkPanel() {}

Vtk::LayoutStrategy VtkPanel::layoutStrategy() const
{
    return m_layoutStrategyComboBox->currentData().value<Vtk::LayoutStrategy>();
}

Vtk::StereoMode VtkPanel::stereoMode() const
{
    return m_stereoModeComboBox->currentData().value<Vtk::StereoMode>();
}

Vtk::ThemeType VtkPanel::themeType() const
{
    return m_themeTypeComboBox->currentData().value<Vtk::ThemeType>();
}

bool VtkPanel::showNodeLabel() const
{
    return m_showNodeLabelCheckBox->isChecked();
}

bool VtkPanel::showEdgeLabel() const
{
    return m_showEdgeLabelCheckBox->isChecked();
}

bool VtkPanel::showEdgeArrow() const
{
    return m_showEdgeArrowCheckBox->isChecked();
}

void VtkPanel::setLayoutStrategy(Vtk::LayoutStrategy strategy)
{
    if (layoutStrategy() == strategy)
        return;
    auto index = m_layoutStrategyComboBox->findData(QVariant::fromValue(strategy));
    m_layoutStrategyComboBox->setCurrentIndex(index);
}

void VtkPanel::setStereoMode(Vtk::StereoMode mode)
{
    if (stereoMode() == mode)
        return;
    auto index = m_stereoModeComboBox->findData(QVariant::fromValue(mode));
    m_stereoModeComboBox->setCurrentIndex(index);
}

void VtkPanel::setTheme(Vtk::ThemeType type)
{
    if (themeType() == type)
        return;
    auto index = m_themeTypeComboBox->findData(QVariant::fromValue(type));
    m_themeTypeComboBox->setCurrentIndex(index);
}

void GammaRay::VtkPanel::setShowNodeLabel(bool show)
{
    if (showNodeLabel() == show)
        return;
    m_showNodeLabelCheckBox->setChecked(show);
}

void VtkPanel::setShowEdgeLabel(bool show)
{
    if (showEdgeLabel() == show)
        return;
    m_showEdgeLabelCheckBox->setChecked(show);
}

void VtkPanel::setShowEdgeArrow(bool show)
{
    if (showEdgeArrow() == show)
        return;
    m_showEdgeArrowCheckBox->setChecked(show);
}

void VtkPanel::createLayoutStrategyComboBox()
{
    auto box = new QComboBox(this);
    // clang-format off
    box->addItem(tr("Span Tree"),
                 QVariant::fromValue(Vtk::LayoutStrategy::SpanTree));
    box->addItem(tr("Force Directed 2D"),
                 QVariant::fromValue(Vtk::LayoutStrategy::ForceDirected2D));
    box->addItem(tr("Force Directed 3D"),
                 QVariant::fromValue(Vtk::LayoutStrategy::ForceDirected3D));
    box->addItem(tr("Simple 2D"),
                 QVariant::fromValue(Vtk::LayoutStrategy::Simple2D));
    box->addItem(tr("Clustering 2D"),
                 QVariant::fromValue(Vtk::LayoutStrategy::Clustering2D));
    box->addItem(tr("Community 2D"),
                 QVariant::fromValue(Vtk::LayoutStrategy::Community2D));
    box->addItem(tr("Fast 2D"),
                 QVariant::fromValue(Vtk::LayoutStrategy::Fast2D));
    box->addItem(tr("Circular"),
                 QVariant::fromValue(Vtk::LayoutStrategy::Circular));
    box->addItem(tr("Tree"),
                 QVariant::fromValue(Vtk::LayoutStrategy::Tree));
    box->addItem(tr("Cone"),
                 QVariant::fromValue(Vtk::LayoutStrategy::Cone));
    box->addItem(tr("Random"),
                 QVariant::fromValue(Vtk::LayoutStrategy::Random));
    box->addItem(tr("Assign Coordinates"),
                 QVariant::fromValue(Vtk::LayoutStrategy::AssignCoordinates));
    box->addItem(tr("Attribute Clustering 2D"),
                 QVariant::fromValue(Vtk::LayoutStrategy::AttributeClustering2D));
    box->addItem(tr("Constrained 2D"),
                 QVariant::fromValue(Vtk::LayoutStrategy::Constrained2D));
    box->addItem(tr("Simple 3D Circles"),
                 QVariant::fromValue(Vtk::LayoutStrategy::Simple3DCircles));
    // clang-format on
    m_layoutStrategyComboBox = box;
}

void VtkPanel::createStereoModeComboBox()
{
    auto box = new QComboBox(this);
    // clang-format off
    box->addItem(tr("Off"),
                 QVariant::fromValue<>(Vtk::StereoMode::Off));
    box->addItem(tr("Crystal Eyes"),
                 QVariant::fromValue<>(Vtk::StereoMode::CrystalEyes));
    box->addItem(tr("Red/Blue"),
                 QVariant::fromValue<>(Vtk::StereoMode::RedBlue));
    box->addItem(tr("Interlaced"),
                 QVariant::fromValue<>(Vtk::StereoMode::Interlaced));
    box->addItem(tr("Left"),
                 QVariant::fromValue<>(Vtk::StereoMode::Left));
    box->addItem(tr("Right"),
                 QVariant::fromValue<>(Vtk::StereoMode::Right));
    box->addItem(tr("Dresden"),
                 QVariant::fromValue<>(Vtk::StereoMode::Dresden));
    box->addItem(tr("Anaglyph"),
                 QVariant::fromValue<>(Vtk::StereoMode::Anaglypth));
    box->addItem(tr("Checkboard"),
                 QVariant::fromValue<>(Vtk::StereoMode::CheckerBoard));
    // clang-format on
    m_stereoModeComboBox = box;
}

void VtkPanel::createThemeComboBox()
{
    auto box = new QComboBox(this);
    // clang-format off
    box->addItem(tr("Ocean"),
                 QVariant::fromValue<>(Vtk::ThemeType::Ocean));
    box->addItem(tr("Mellow"),
                 QVariant::fromValue<>(Vtk::ThemeType::Mellow));
    box->addItem(tr("Neon"),
                 QVariant::fromValue<>(Vtk::ThemeType::Neon));
    // clang-format on
    m_themeTypeComboBox = box;
}
