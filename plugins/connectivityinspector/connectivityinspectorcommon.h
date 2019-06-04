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

#ifndef GAMMARAY_CONNECTIVITYINSPECTOR_CONNECTIVITYINSECTORCOMMON_H
#define GAMMARAY_CONNECTIVITYINSPECTOR_CONNECTIVITYINSECTORCOMMON_H

#include <qnamespace.h>

namespace GammaRay {
extern const char *ObjectVisualizerConnectionModelId;
extern const char *ObjectVisualizerConnectionTypeModelId;
extern const char *ObjectVisualizerThreadModelId;
extern const char *ObjectVisualizerClassModelId;
extern const char *ObjectVisualizerObjectModelId;
extern const char *ConnectivityInspectorBaseDomain;

enum {
    MetricColumRole = Qt::UserRole + 100,
    MetricMaxRole,
};

} // namespace GammaRay

#endif // GAMMARAY_CONNECTIVITYINSPECTOR_CONNECTIVITYINSECTORCOMMON_H
