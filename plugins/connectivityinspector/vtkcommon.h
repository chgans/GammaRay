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

#ifndef GAMMARAY_CONNECTIVITYINSPECTOR_VTKCOMMON_H
#define GAMMARAY_CONNECTIVITYINSPECTOR_VTKCOMMON_H

#include <QtCore/QMetaType>

namespace GammaRay {
namespace Vtk {

enum class LayoutStrategy {
    AssignCoordinates = 0,
    AttributeClustering2D,
    Circular,
    Clustering2D,
    Community2D,
    Cone,
    Constrained2D,
    /* CosmicTree only works on vtkTree unless VTK_USE_BOOST is on */
    Fast2D,
    ForceDirected2D,
    ForceDirected3D,
    Random,
    Simple2D,
    Simple3DCircles,
    SpanTree,
    Tree,
    /* TreeOrbit only works on vtkTree unless VTK_USE_BOOST is on */
};

enum class ThemeType { Ocean = 0, Mellow, Neon };

// Has to match VTK_STEREO_FOO_BAR in vtkRenderWindow.h
enum class StereoMode {
    Off = 0,
    CrystalEyes,
    RedBlue,
    Interlaced,
    Left,
    Right,
    Dresden,
    Anaglypth,
    CheckerBoard
};

} // namespace Vtk
} // namespace GammaRay

Q_DECLARE_METATYPE(GammaRay::Vtk::StereoMode)
Q_DECLARE_METATYPE(GammaRay::Vtk::LayoutStrategy)
Q_DECLARE_METATYPE(GammaRay::Vtk::ThemeType)

#endif // GAMMARAY_CONNECTIVITYINSPECTOR_VTKCOMMON_H
