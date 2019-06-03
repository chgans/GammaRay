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

#include "connectiontypemodel.h"

using namespace GammaRay;

namespace {
const QVector<const char *> s_typeNames{
    "Auto",
    "Direct",
    "Queued",
    "Blocking queued",
    "Unique auto",
    "Unique direct",
    "Unique queued",
    "Unique blocking queued",
};
} // namespace

const QVector<int> ConnectionTypeModel::s_data{
    Qt::AutoConnection,
    Qt::DirectConnection,
    Qt::QueuedConnection,
    Qt::BlockingQueuedConnection,
    Qt::AutoConnection | Qt::UniqueConnection,
    Qt::DirectConnection | Qt::UniqueConnection,
    Qt::QueuedConnection | Qt::UniqueConnection,
    Qt::BlockingQueuedConnection | Qt::UniqueConnection,
};

ConnectionTypeModel::ConnectionTypeModel(QObject *parent)
    : QAbstractTableModel(parent)
{}

ConnectionTypeModel::~ConnectionTypeModel() = default;

int ConnectionTypeModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return s_data.count();
}

int ConnectionTypeModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 1;
}

QVariant ConnectionTypeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};
    const int row = index.row();
    if (row < 0 || row >= s_data.count())
        return {};
    if (index.column() != 0)
        return {};
    if (role == Qt::DisplayRole)
        return tr(s_typeNames.value(row));
    if (role == TypeRole)
        return s_data.value(row);
    return {};
}

QVariant GammaRay::ConnectionTypeModel::headerData(int section,
                                                   Qt::Orientation orientation,
                                                   int role) const
{
    if (role != Qt::DisplayRole)
        return {};
    if (orientation != Qt::Horizontal)
        return {};
    if (section != 0)
        return {};
    return "Type";
}
