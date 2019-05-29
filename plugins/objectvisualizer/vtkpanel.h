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

#ifndef GAMMARAY_OBJECTVISUALIZER_VTKPANEL_H
#define GAMMARAY_OBJECTVISUALIZER_VTKPANEL_H

#include "vtkcommon.h"

#include <QToolBar>

QT_BEGIN_NAMESPACE
class QComboBox;
class QCheckBox;
QT_END_NAMESPACE

namespace GammaRay {
class VtkWidget;

class VtkPanel : public QToolBar
{
    Q_OBJECT

public:
    explicit VtkPanel(QWidget *parent = nullptr);
    ~VtkPanel() override;

    Vtk::LayoutStrategy layoutStrategy() const;
    Vtk::StereoMode stereoMode() const;
    Vtk::ThemeType themeType() const;
    bool showNodeLabel() const;
    bool showEdgeLabel() const;
    bool showEdgeArrow() const;

public slots:
    void setLayoutStrategy(GammaRay::Vtk::LayoutStrategy strategy);
    void setStereoMode(GammaRay::Vtk::StereoMode mode);
    void setTheme(GammaRay::Vtk::ThemeType type);
    void setShowNodeLabel(bool show);
    void setShowEdgeLabel(bool show);
    void setShowEdgeArrow(bool show);

signals:
    void layoutStrategyChanged(Vtk::LayoutStrategy strategy);
    void stereoModeChanged(Vtk::StereoMode mode);
    void themeChanged(Vtk::ThemeType themeType);
    void showNodeLabelChanged(bool show);
    void showEdgeLabelChanged(bool show);
    void showEdgeArrowChanged(bool show);

private:
    QComboBox *m_layoutStrategyComboBox;
    QComboBox *m_stereoModeComboBox;
    QComboBox *m_themeTypeComboBox;
    QCheckBox *m_showNodeLabelCheckBox;
    QCheckBox *m_showEdgeLabelCheckBox;
    QCheckBox *m_showEdgeArrowCheckBox;

    void createLayoutStrategyComboBox();
    void createStereoModeComboBox();
    void createThemeComboBox();
};

} // namespace GammaRay

#endif // GAMMARAY_VTKPANEL_H
