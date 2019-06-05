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

#include "filterproxymodel.h"

#include "connectivityinspectorcommon.h"

#include <functional>

using namespace GammaRay;

FilterProxyModelBase::FilterProxyModelBase(QObject *parent)
    : KExtraColumnsProxyModel(parent) {
    appendColumn("Count");
    appendColumn("Record");
    appendColumn("Show");
}

FilterProxyModelBase::~FilterProxyModelBase() = default;

QVariant FilterProxyModelBase::extraColumnData(const QModelIndex &parent,
                                               int row, int extraColumn,
                                               int role) const {
    if (!sourceModel())
        return {};
    const auto srcIndex = sourceModel()->index(row, 0, mapToSource(parent));
    if (role == MetricColumRole) {
        return proxyColumnForExtraColumn(CountColumn);
    }
    if (role == MetricMaxRole) {
        switch (extraColumn) {
        case CountColumn:
            return int(m_maxCount);
        }
    } else if (role == Qt::DisplayRole) {
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

bool FilterProxyModelBase::setExtraColumnData(const QModelIndex &parent,
                                              int row, int extraColumn,
                                              const QVariant &value, int role) {
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

Qt::ItemFlags FilterProxyModelBase::extraColumnFlags(int extraColumn) const {
    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    if (extraColumn == IsRecordingColumn || extraColumn == IsVisibleColumn)
        flags |= Qt::ItemIsUserCheckable;

    return flags;
}

QMap<int, QVariant>
FilterProxyModelBase::extraItemData(const QModelIndex &index) const {
    QMap<int, QVariant> map = QAbstractItemModel::itemData(index);
    map.insert(MetricColumRole, data(index, MetricColumRole));
    map.insert(MetricMaxRole, data(index, MetricMaxRole));
    return map;
}

void FilterProxyModelBase::setSourceModel(QAbstractItemModel *model) {
    if (sourceModel())
        finaliseRecordingModel();
    QIdentityProxyModel::setSourceModel(model);
    if (sourceModel())
        initialiseRecordingModel();
}

void FilterProxyModelBase::resetCounts() {
    if (!sourceModel())
        return;

    beginResetModel();
    for (auto &data : m_data)
        data.count = 0;
    m_maxCount = 0;
    endResetModel();
}

void FilterProxyModelBase::increaseCount(const QModelIndex &index) {
    Q_ASSERT(m_data.contains(index));
    m_data[index].count++;
    m_maxCount = std::max(m_maxCount, m_data.value(index).count);
    const auto proxyIndex = mapFromSource(index);
    extraColumnDataChanged(proxyIndex.parent(), proxyIndex.row(), CountColumn,
                           {Qt::DisplayRole});
}

void FilterProxyModelBase::decreaseCount(const QModelIndex &index) {
    Q_ASSERT(m_data.contains(index));
    m_data[index].count--;
    // max count?
    const auto proxyIndex = mapFromSource(index);
    extraColumnDataChanged(proxyIndex.parent(), proxyIndex.row(), CountColumn,
                           {Qt::DisplayRole});
}

quint64 FilterProxyModelBase::recordCount(const QModelIndex &index) const {
    Q_ASSERT(m_data.contains(index));
    return m_data.value(index).count;
}

bool FilterProxyModelBase::isRecording(const QModelIndex &index) const {
    Q_ASSERT(m_data.contains(index));
    return m_data.value(index).isRecording;
}

void FilterProxyModelBase::setIsRecording(const QModelIndex &index,
                                          bool enabled) {
    Q_ASSERT(m_data.contains(index));
    m_data[index].isRecording = enabled;
}

bool FilterProxyModelBase::isVisible(const QModelIndex &index) const {
    Q_ASSERT(m_data.contains(index));
    return m_data.value(index).isVisible;
}

void FilterProxyModelBase::setIsVisible(const QModelIndex &index,
                                        bool enabled) {
    Q_ASSERT(m_data.contains(index));
    m_data[index].isVisible = enabled;
}

void FilterProxyModelBase::recordAll() {
    if (!sourceModel())
        return;

    beginResetModel();
    for (auto &data : m_data)
        data.isRecording = true;
    endResetModel();
}

void FilterProxyModelBase::recordNone() {
    if (!sourceModel())
        return;

    beginResetModel();
    for (auto &data : m_data)
        data.isRecording = false;
    endResetModel();
}

void FilterProxyModelBase::showAll() {
    if (!sourceModel())
        return;

    beginResetModel();
    for (auto &data : m_data)
        data.isVisible = true;
    endResetModel();
}

void FilterProxyModelBase::showNone() {
    if (!sourceModel())
        return;

    beginResetModel();
    for (auto &data : m_data)
        data.isVisible = false;
    endResetModel();
}

void FilterProxyModelBase::visitIndex(const QModelIndex &sourceIndex,
                                      IndexVisitor visitor) {
    const int rowCount = sourceModel()->rowCount(sourceIndex);
    for (int row = 0; row < rowCount; ++row) {
        const auto index = sourceModel()->index(row, 0, sourceIndex);
        visitor(index);
        visitIndex(index, visitor);
    }
}

void FilterProxyModelBase::initialiseRecordingModel() {
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

void FilterProxyModelBase::finaliseRecordingModel() {
    sourceModel()->disconnect(this);
    clearRecordingData();
}
