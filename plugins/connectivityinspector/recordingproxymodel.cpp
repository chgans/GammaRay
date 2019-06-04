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

#include "recordingproxymodel.h"

#include <functional>

using namespace GammaRay;

RecordingProxyModelBase::RecordingProxyModelBase(QObject *parent)
    : KExtraColumnsProxyModel(parent)
{
    appendColumn("Count");
    appendColumn("Record");
    appendColumn("Show");
}

RecordingProxyModelBase::~RecordingProxyModelBase() = default;

QVariant RecordingProxyModelBase::extraColumnData(const QModelIndex &parent,
                                                  int row,
                                                  int extraColumn,
                                                  int role) const
{
    if (!sourceModel())
        return {};
    const auto srcIndex = sourceModel()->index(row, 0, mapToSource(parent));
    if (role == Qt::DisplayRole) {
        switch (extraColumn) {
        case CountColumn:
            return recordCount(srcIndex);
        }
    } else if (role == Qt::CheckStateRole) {
        switch (extraColumn) {
        case IsRecordingColumn:
            return isRecording(srcIndex) ? Qt::Checked : Qt::Unchecked;
        case IsVisibleColumn:
            return isVisible(srcIndex) ? Qt::Checked : Qt::Unchecked;
        }
    }

    return {};
}

bool RecordingProxyModelBase::setExtraColumnData(
    const QModelIndex &parent, int row, int extraColumn, const QVariant &value, int role)
{
    if (role != Qt::CheckStateRole)
        return false;

    const auto srcIndex = sourceModel()->index(row, 0, mapToSource(parent));
    const bool enabled = value.toInt() == Qt::Checked;
    if (extraColumn == IsRecordingColumn) {
        setIsRecording(srcIndex, enabled);
    } else if (extraColumn == IsVisibleColumn) {
        setIsVisible(srcIndex, enabled);
    } else
        return false;
    extraColumnDataChanged(parent, row, extraColumn, {role});
    return true;
}

Qt::ItemFlags RecordingProxyModelBase::extraColumnFlags(int extraColumn) const
{
    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    if (extraColumn == IsRecordingColumn || extraColumn == IsVisibleColumn)
        flags |= Qt::ItemIsUserCheckable;

    return flags;
}

void RecordingProxyModelBase::setSourceModel(QAbstractItemModel *model)
{
    if (sourceModel())
        finaliseRecordingModel();
    QIdentityProxyModel::setSourceModel(model);
    if (sourceModel())
        initialiseRecordingModel();
}

void RecordingProxyModelBase::resetCounts() {
    if (!sourceModel())
        return;

    beginResetModel();
    for (auto &data : m_data)
        data.count = 0;
    endResetModel();
}

void RecordingProxyModelBase::increaseCount(const QModelIndex &index) {
    Q_ASSERT(m_data.contains(index));
    m_data[index].count++;
    const auto proxyIndex = mapFromSource(index);
    extraColumnDataChanged(proxyIndex.parent(), proxyIndex.row(), CountColumn,
                           {Qt::DisplayRole});
}

void RecordingProxyModelBase::decreaseCount(const QModelIndex &index) {
    Q_ASSERT(m_data.contains(index));
    m_data[index].count--;
    const auto proxyIndex = mapFromSource(index);
    extraColumnDataChanged(proxyIndex.parent(), proxyIndex.row(), CountColumn,
                           {Qt::DisplayRole});
}

quint64 RecordingProxyModelBase::recordCount(const QModelIndex &index) const {
    Q_ASSERT(m_data.contains(index));
    return m_data.value(index).count;
}

bool RecordingProxyModelBase::isRecording(const QModelIndex &index) const {
    Q_ASSERT(m_data.contains(index));
    return m_data.value(index).isRecording;
}

void RecordingProxyModelBase::setIsRecording(const QModelIndex &index,
                                             bool enabled) {
    Q_ASSERT(m_data.contains(index));
    m_data[index].isRecording = enabled;
}

bool RecordingProxyModelBase::isVisible(const QModelIndex &index) const {
    Q_ASSERT(m_data.contains(index));
    return m_data.value(index).isVisible;
}

void RecordingProxyModelBase::setIsVisible(const QModelIndex &index,
                                           bool enabled) {
    Q_ASSERT(m_data.contains(index));
    m_data[index].isVisible = enabled;
}

void RecordingProxyModelBase::recordAll()
{
    if (!sourceModel())
        return;

    beginResetModel();
    for (auto &data : m_data)
        data.isRecording = true;
    endResetModel();
}

void RecordingProxyModelBase::recordNone()
{
    if (!sourceModel())
        return;

    beginResetModel();
    for (auto &data : m_data)
        data.isRecording = false;
    endResetModel();
}

void RecordingProxyModelBase::showAll()
{
    if (!sourceModel())
        return;

    beginResetModel();
    for (auto &data : m_data)
        data.isVisible = true;
    endResetModel();
}

void RecordingProxyModelBase::showNone()
{
    if (!sourceModel())
        return;

    beginResetModel();
    for (auto &data : m_data)
        data.isVisible = false;
    endResetModel();
}

void RecordingProxyModelBase::visitIndex(const QModelIndex &sourceIndex, IndexVisitor visitor)
{
    const int rowCount = sourceModel()->rowCount(sourceIndex);
    for (int row = 0; row < rowCount; ++row) {
        const auto index = sourceModel()->index(row, 0, sourceIndex);
        visitor(index);
        visitIndex(index, visitor);
    }
}

void RecordingProxyModelBase::initialiseRecordingModel()
{
    visitIndex(QModelIndex(), [this](const QModelIndex &index) {
        m_data.insert(index, {});
        addRecordingData(index);
    });
    connect(sourceModel(),
            &QAbstractItemModel::rowsInserted,
            this,
            [this](const QModelIndex &parent, int first, int last) {
                for (int row = first; row <= last; ++row) {
                    const auto index = sourceModel()->index(row, 0, parent);
                    Q_ASSERT(!m_data.contains(index));
                    m_data.insert(index, {});
                    addRecordingData(index);
                }
            });
    connect(sourceModel(), &QAbstractItemModel::rowsAboutToBeRemoved, this,
            [this](const QModelIndex &parent, int first, int last) {
                for (int row = first; row <= last; ++row) {
                    const auto index = sourceModel()->index(row, 0, parent);
                    Q_ASSERT(m_data.contains(index));
                    m_data.remove(index);
                    removeRecordingData(index);
                }
            });
    connect(sourceModel(), &QAbstractItemModel::modelReset, this, [this]() {
        m_data = QHash<QPersistentModelIndex, RecordingData>();
        clearRecordingData();
    });
}

void RecordingProxyModelBase::finaliseRecordingModel()
{
    sourceModel()->disconnect(this);
    clearRecordingData();
}
