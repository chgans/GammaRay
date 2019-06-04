/*
  This file is part of GammaRay, the Qt application inspection and
  manipulation tool.

  Copyright (C) 2019 Klarälvdalens Datakonsult AB, a KDAB Group company,
  info@kdab.com Author: Tim Henning <tim.henning@kdab.com>

  Licensees holding valid commercial KDAB GammaRay licenses may use this file in
  accordance with GammaRay Commercial License Agreement provided with the
  Software.

  Contact info@kdab.com if any conditions of this licensing are not clear to
  you.

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

#include "countdecoratorproxymodel.h"
#include "connectivityinspectorcommon.h"

#include <ui/uiintegration.h>

#include <QColor>

using namespace GammaRay;

namespace {

// 1 / GRADIENT_SCALE_FACTOR is yellow
// 2 / GRADIENT_SCALE_FACTOR and beyond is red
const int GRADIENT_SCALE_FACTOR = 4;

QColor colorForRatio(double ratio) {
    const auto red = qBound<qreal>(0.0, ratio * GRADIENT_SCALE_FACTOR, 0.5);
    const auto green =
        qBound<qreal>(0.0, 1 - ratio * GRADIENT_SCALE_FACTOR, 0.5);
    auto color = QColor(int(255 * red), int(255 * green), 0);
    if (!UiIntegration::hasDarkUI())
        return color.lighter(300);
    return color;
}

} // namespace

CountDecoratorProxyModel::CountDecoratorProxyModel(QObject *parent)
    : QIdentityProxyModel(parent), m_metricColumnRole(MetricColumRole),
      m_metricMaxRole(MetricMaxRole) {}

CountDecoratorProxyModel::~CountDecoratorProxyModel() {}

QVariant CountDecoratorProxyModel::data(const QModelIndex &index,
                                        int role) const {

    if (!sourceModel() || !index.isValid())
        return QVariant();

    // FIXME: -3
    if (role != Qt::BackgroundRole || index.column() != columnCount() - 3)
        return QIdentityProxyModel::data(index, role);

    const int maxCount =
        QIdentityProxyModel::data(index, m_metricMaxRole).toInt();
    const int count = QIdentityProxyModel::data(index, Qt::DisplayRole).toInt();
    if (maxCount <= 0 || count <= 0) {
        return QVariant();
    }
    const double ratio = double(count) / double(maxCount);

    return colorForRatio(ratio);
}
