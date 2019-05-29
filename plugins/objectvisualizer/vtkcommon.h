/*
  vtkcommon.h

  This file is part of QGraphViz, a Qt wrapper around GraphViz libraries.

  Copyright (C) 2019

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

#ifndef VTKCOMMON_H
#define VTKCOMMON_H

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

#endif // VTKCOMMON_H
