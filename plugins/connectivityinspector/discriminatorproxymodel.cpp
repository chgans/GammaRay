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

#include "discriminatorproxymodel.h"

#include "connectivityinspectorcommon.h"

#include <functional>

using namespace GammaRay;

DiscriminatorProxyModelBase::DiscriminatorProxyModelBase(QObject *parent)
    : KExtraColumnsProxyModel(parent)
{
    appendColumn("Connectivity");
    appendColumn("Enable");
    appendColumn("Filter");
}

DiscriminatorProxyModelBase::~DiscriminatorProxyModelBase() = default;

QVariant DiscriminatorProxyModelBase::extraColumnData(const QModelIndex &parent,
                                                      int row,
                                                      int extraColumn,
                                                      int role) const
{
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
            return connectivityCount(srcIndex);
        }
    } else if (role == Qt::CheckStateRole) {
        switch (extraColumn) {
        case IsDiscrimatingColumn:
            return isDiscriminating(srcIndex) ? Qt::Checked : Qt::Unchecked;
        case IsFilteringColumn:
            return isFiltering(srcIndex) ? Qt::Checked : Qt::Unchecked;
        }
    }

    return {};
}

bool DiscriminatorProxyModelBase::setExtraColumnData(
    const QModelIndex &parent, int row, int extraColumn, const QVariant &value, int role)
{
    if (role != Qt::CheckStateRole)
        return false;

    const auto srcIndex = sourceModel()->index(row, 0, mapToSource(parent));
    const bool enabled = value.toInt() == Qt::Checked;
    if (extraColumn == IsDiscrimatingColumn) {
        setIsDiscriminating(srcIndex, enabled);
    } else if (extraColumn == IsFilteringColumn) {
        if (isDiscriminating(srcIndex))
            setIsFiltering(srcIndex, enabled);
    } else
        return false;
    extraColumnDataChanged(parent, row, IsDiscrimatingColumn, {role});
    extraColumnDataChanged(parent, row, IsFilteringColumn, {role});
    return true;
}

Qt::ItemFlags DiscriminatorProxyModelBase::extraColumnFlags(const QModelIndex &parent,
                                                            int row,
                                                            int extraColumn) const
{
    constexpr auto checkFlags = Qt::ItemIsUserCheckable | Qt::ItemIsAutoTristate;

    switch (extraColumn) {
    case CountColumn:
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    case IsDiscrimatingColumn:
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | checkFlags;
    case IsFilteringColumn: {
        const auto srcIndex = sourceModel()->index(row, 0, mapToSource(parent));
        if (isDiscriminating(srcIndex))
            return Qt::ItemIsEnabled | Qt::ItemIsSelectable | checkFlags;
        return Qt::ItemIsSelectable | checkFlags;
    }
    }
    Q_ASSERT(false);
    return {};
}

QMap<int, QVariant> DiscriminatorProxyModelBase::extraItemData(const QModelIndex &index) const
{
    QMap<int, QVariant> map = QAbstractItemModel::itemData(index);
    map.insert(MetricColumRole, data(index, MetricColumRole));
    map.insert(MetricMaxRole, data(index, MetricMaxRole));
    return map;
}

void DiscriminatorProxyModelBase::setSourceModel(QAbstractItemModel *model)
{
    if (sourceModel())
        finaliseDiscriminator();
    QIdentityProxyModel::setSourceModel(model);
    if (sourceModel())
        initialiseDiscriminator();
}

void DiscriminatorProxyModelBase::resetCounts()
{
    if (!sourceModel())
        return;

    beginResetModel();
    for (auto &data : m_data)
        data.count = 0;
    m_maxCount = 0;
    endResetModel();
}

void DiscriminatorProxyModelBase::increaseConnectivity(const QModelIndex &index)
{
    Q_ASSERT(m_data.contains(index));
    m_data[index].count++;
    m_maxCount = std::max(m_maxCount, m_data.value(index).count);
    const auto proxyIndex = mapFromSource(index);
    extraColumnDataChanged(proxyIndex.parent(), proxyIndex.row(), CountColumn,
                           {Qt::DisplayRole});
}

void DiscriminatorProxyModelBase::decreaseConnectivity(const QModelIndex &index)
{
    Q_ASSERT(m_data.contains(index));
    m_data[index].count--;
    // max count?
    const auto proxyIndex = mapFromSource(index);
    extraColumnDataChanged(proxyIndex.parent(), proxyIndex.row(), CountColumn,
                           {Qt::DisplayRole});
}

quint64 DiscriminatorProxyModelBase::connectivityCount(const QModelIndex &index) const
{
    Q_ASSERT(m_data.contains(index));
    return m_data.value(index).count;
}

void DiscriminatorProxyModelBase::setEnabled(bool enabled)
{
    if (m_enabled == enabled)
        return;
    m_enabled = enabled;
    // TODO: ?
    // emit isEnabledChanged(m_enabled);
}

bool DiscriminatorProxyModelBase::isDiscriminating(const QModelIndex &index) const
{
    Q_ASSERT(m_data.contains(index));
    return m_data.value(index).isDiscriminating;
}

void DiscriminatorProxyModelBase::setIsDiscriminating(const QModelIndex &index, bool enabled)
{
    Q_ASSERT(m_data.contains(index));
    m_data[index].isDiscriminating = enabled;
}

bool DiscriminatorProxyModelBase::isFiltering(const QModelIndex &index) const
{
    Q_ASSERT(m_data.contains(index));
    return m_data.value(index).isFiltering;
}

void DiscriminatorProxyModelBase::setIsFiltering(const QModelIndex &index, bool enabled)
{
    Q_ASSERT(m_data.contains(index));
    m_data[index].isFiltering = enabled;
}

void DiscriminatorProxyModelBase::discriminateAll()
{
    if (!sourceModel())
        return;

    beginResetModel();
    for (auto &data : m_data)
        data.isDiscriminating = true;
    endResetModel();
}

void DiscriminatorProxyModelBase::discriminateNone()
{
    if (!sourceModel())
        return;

    beginResetModel();
    for (auto &data : m_data)
        data.isDiscriminating = false;
    endResetModel();
}

void DiscriminatorProxyModelBase::filterAll()
{
    if (!sourceModel())
        return;

    beginResetModel();
    for (auto &data : m_data) {
        if (data.isDiscriminating)
            data.isFiltering = true;
    }
    endResetModel();
}

void DiscriminatorProxyModelBase::filterNone()
{
    if (!sourceModel())
        return;

    beginResetModel();
    for (auto &data : m_data)
        if (data.isDiscriminating)
            data.isFiltering = false;
    endResetModel();
}

void DiscriminatorProxyModelBase::visitIndex(const QModelIndex &sourceIndex, IndexVisitor visitor)
{
    const int rowCount = sourceModel()->rowCount(sourceIndex);
    for (int row = 0; row < rowCount; ++row) {
        const auto index = sourceModel()->index(row, 0, sourceIndex);
        visitor(index);
        visitIndex(index, visitor);
    }
}

void DiscriminatorProxyModelBase::initialiseDiscriminator()
{
    visitIndex(QModelIndex(), [this](const QModelIndex &index) {
        m_data.insert(index, {});
        addItemData(index);
    });
    connect(sourceModel(),
            &QAbstractItemModel::rowsInserted,
            this,
            [this](const QModelIndex &parent, int first, int last) {
                for (int row = first; row <= last; ++row) {
                    const auto index = sourceModel()->index(row, 0, parent);
                    Q_ASSERT(!m_data.contains(index));
                    m_data.insert(index, ItemData());
                    addItemData(index);
                }
            });
    connect(sourceModel(), &QAbstractItemModel::rowsAboutToBeRemoved, this,
            [this](const QModelIndex &parent, int first, int last) {
                for (int row = first; row <= last; ++row) {
                    const auto index = sourceModel()->index(row, 0, parent);
                    Q_ASSERT(m_data.contains(index));
                    m_data.remove(index);
                    removeItemData(index);
                }
            });
    connect(sourceModel(), &QAbstractItemModel::modelAboutToBeReset, this, [this]() {
        m_data.clear();
        clearItemData();
    });
}

void DiscriminatorProxyModelBase::finaliseDiscriminator()
{
    sourceModel()->disconnect(this);
    m_data.clear();
    clearItemData();
}

FilterProxyModel::FilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{}

void FilterProxyModel::setDiscriminatorModel(DiscriminatorProxyModelBase *model)
{
    m_discriminator = model;
}

bool FilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    Q_ASSERT(m_discriminator);
    return m_discriminator->isAccepting(source_parent.child(source_row, 0));
}

FilterProxyModel::~FilterProxyModel() = default;

void DiscriminatorBase::setEnabled(bool enabled)
{
    Q_ASSERT(m_discriminatorProxyModel);
    m_discriminatorProxyModel->setEnabled(enabled);
}

void DiscriminatorBase::discriminateAll()
{
    Q_ASSERT(m_discriminatorProxyModel);
    m_discriminatorProxyModel->discriminateAll();
}

void DiscriminatorBase::discriminateNone()
{
    Q_ASSERT(m_discriminatorProxyModel);
    m_discriminatorProxyModel->discriminateNone();
}

void DiscriminatorBase::filterAll()
{
    Q_ASSERT(m_discriminatorProxyModel);
    m_discriminatorProxyModel->filterAll();
}

void DiscriminatorBase::filterNone()
{
    Q_ASSERT(m_discriminatorProxyModel);
    m_discriminatorProxyModel->filterNone();
}

const QAbstractItemModel *DiscriminatorBase::filteredModel() const
{
    return m_filterProxyModel;
}

QMap<int, QVariant> GammaRay::FilterProxyModel::itemData(const QModelIndex &index) const
{
    return sourceModel()->itemData(mapToSource(index));
}
