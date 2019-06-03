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
int connectionTypeToRow(int type)
{
    Q_ASSERT(type >= 0 && type < ConnectionTypeModel::RowCount);
    return type;
}
} // namespace

const int ConnectionTypeModel::RowCount = 8;
const QVector<ConnectionTypeModel::Data> ConnectionTypeModel::s_initialData{
    {0, Qt::AutoConnection, 0, 0},
    {0, Qt::DirectConnection, 0, 0},
    {0, Qt::QueuedConnection, 0, 0},
    {0, Qt::BlockingQueuedConnection, 0, 0},
    {0, Qt::AutoConnection | Qt::UniqueConnection, 0, 0},
    {0, Qt::DirectConnection | Qt::UniqueConnection, 0, 0},
    {0, Qt::QueuedConnection | Qt::UniqueConnection, 0, 0},
    {0, Qt::BlockingQueuedConnection | Qt::UniqueConnection, 0, 0},
};

ConnectionTypeModel::ConnectionTypeModel(QObject *parent)
    : QAbstractTableModel(parent)
    , m_data(s_initialData)
{
    Q_ASSERT(s_initialData.count() == RowCount);
    Q_ASSERT(s_typeNames.count() == RowCount);
}

ConnectionTypeModel::~ConnectionTypeModel() = default;

int ConnectionTypeModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return RowCount;
}

int ConnectionTypeModel::columnCount(const QModelIndex & /*parent*/) const
{
    return ColumnCount;
}

QVariant ConnectionTypeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};
    const int row = index.row();
    if (row < 0 || row >= RowCount)
        return {};

    const int column = index.column();
    if (role == Qt::DisplayRole) {
        switch (column) {
        case TypeColumn:
            return tr(s_typeNames.value(row));
        case CountColumn:
            return m_data.value(row).count;
        }
    } else if (role == Qt::CheckStateRole) {
        switch (index.column()) {
        case IsRecordingColumn:
            return m_data[row].isRecording ? Qt::Checked : Qt::Unchecked;
        case IsVisibleColumn:
            return m_data[row].isVisible ? Qt::Checked : Qt::Unchecked;
        }
    }

    return {};
}

Qt::ItemFlags ConnectionTypeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemFlag::NoItemFlags;

    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    if (index.column() == IsRecordingColumn || index.column() == IsVisibleColumn)
        flags |= Qt::ItemIsUserCheckable;

    return flags;
}

bool ConnectionTypeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;
    if (role != Qt::CheckStateRole)
        return false;
    const auto row = index.row();
    if (row < 0 || row >= RowCount)
        return false;
    const auto column = index.column();
    const auto enabled = value.toInt() == Qt::Checked;
    if (column == IsRecordingColumn) {
        m_data[row].isRecording = enabled;
        emit recordingChanged();
    } else if (column == IsVisibleColumn) {
        m_data[row].isVisible = enabled;
    } else
        return false;
    emit dataChanged(index, index, {Qt::CheckStateRole});
    return true;
}

QVariant GammaRay::ConnectionTypeModel::headerData(int section,
                                                   Qt::Orientation orientation,
                                                   int role) const
{
    if (role != Qt::DisplayRole)
        return {};
    if (orientation != Qt::Horizontal)
        return {};
    switch (section) {
    case TypeColumn:
        return "Type";
    case CountColumn:
        return "Count";
    case IsRecordingColumn:
        return "Record";
    case IsVisibleColumn:
        return "Show";
    default:
        return {};
    }
}

void ConnectionTypeModel::increaseCount(int type)
{
    const int row = connectionTypeToRow(type);
    m_data[row].count++;
    const auto change = index(row, CountColumn);
    emit dataChanged(change, change);
}

void ConnectionTypeModel::decreaseCount(int type)
{
    const int row = connectionTypeToRow(type);
    m_data[row].count--;
    const auto change = index(row, CountColumn);
    emit dataChanged(change, change);
}

void ConnectionTypeModel::resetCounts()
{
    beginResetModel();
    for (int row = 0; row < RowCount; ++row)
        m_data[row].count = 0;
    endResetModel();
}

bool ConnectionTypeModel::isRecording(Qt::ConnectionType type) const
{
    const int row = connectionTypeToRow(type);
    return m_data[row].isRecording;
}

void ConnectionTypeModel::recordAll()
{
    beginResetModel();
    for (int row = 0; row < RowCount; ++row)
        m_data[row].isRecording = true;
    endResetModel();
}

void ConnectionTypeModel::recordNone()
{
    beginResetModel();
    for (int row = 0; row < RowCount; ++row)
        m_data[row].isRecording = false;
    endResetModel();
}

bool ConnectionTypeModel::isVisible(Qt::ConnectionType type) const
{
    const int row = connectionTypeToRow(type);
    return m_data[row].isVisible;
}

void ConnectionTypeModel::showAll()
{
    beginResetModel();
    for (int row = 0; row < RowCount; ++row)
        m_data[row].isVisible = true;
    endResetModel();
}

void ConnectionTypeModel::showNone()
{
    beginResetModel();
    for (int row = 0; row < RowCount; ++row)
        m_data[row].isVisible = false;
    endResetModel();
}
