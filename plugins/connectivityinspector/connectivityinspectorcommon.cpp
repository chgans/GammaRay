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

#include "connectivityinspectorcommon.h"

#include <QString>

namespace GammaRay {
namespace CI { // FIXME

static const char *BaseDomain = "com.kdab.GammaRay.ConnectivityInspector";

QString filterModelId(const QString &name)
{
    return QStringLiteral("%1.Filter.%2.Model").arg(BaseDomain).arg(name);
}

QString filterInterfaceId(const QString &name)
{
    return QStringLiteral("%1.Filter.%2.Interface").arg(BaseDomain).arg(name);
}

QString filteredModelId(const QString &name)
{
    return QStringLiteral("%1.Filter.%2.Output").arg(BaseDomain).arg(name);
}

QString threadFilterName()
{
    return QStringLiteral("Thread");
}

QString objectFilterName()
{
    return QStringLiteral("Object");
}

QString classFilterName()
{
    return QStringLiteral("Class");
}

QString typeFilterName()
{
    return QStringLiteral("Type");
}

QString connectionFilterName()
{
    return QStringLiteral("Connection");
}

} // namespace CI
} // namespace GammaRay
