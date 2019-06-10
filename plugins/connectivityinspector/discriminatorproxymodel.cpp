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

/*****************************************************************************
 * DiscriminatorProxyModelBase
 ****************************************************************************/

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
    Q_ASSERT(m_data.contains(srcIndex));
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
            return m_data[srcIndex].count;
        }
    } else if (role == Qt::CheckStateRole) {
        switch (extraColumn) {
        case IsDiscrimatingColumn:
            return m_data[srcIndex].discriminatingCheckState;
        case IsFilteringColumn:
            return m_data[srcIndex].filteringCheckState;
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
    if (extraColumn == IsDiscrimatingColumn) {
        m_data[srcIndex].discriminatingCheckState =
            value.value<Qt::CheckState>();
    } else if (extraColumn == IsFilteringColumn) {
        m_data[srcIndex].filteringCheckState = value.value<Qt::CheckState>();
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
    constexpr auto checkFlags =
        Qt::ItemIsUserCheckable | Qt::ItemIsAutoTristate;

    const auto srcIndex = sourceModel()->index(row, 0, mapToSource(parent));
    switch (extraColumn) {
    case CountColumn:
        return (Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    case IsDiscrimatingColumn:
        return (Qt::ItemIsEnabled | Qt::ItemIsSelectable | checkFlags);
    case IsFilteringColumn: {
        if (m_data[srcIndex].discriminatingCheckState != Qt::Unchecked)
            return (Qt::ItemIsEnabled | Qt::ItemIsSelectable | checkFlags);
        return (Qt::ItemIsSelectable | checkFlags);
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

bool DiscriminatorProxyModelBase::isAccepting(const QModelIndex &index) const {
    Q_ASSERT(m_data.contains(index));
    const auto &item = m_data[index];
    if (!m_enabled)
        return true;
    if (item.discriminatingCheckState == Qt::Unchecked)
        return true;
    if (item.filteringCheckState == Qt::Unchecked)
        return true;
    return false;
}

void DiscriminatorProxyModelBase::setEnabled(bool enabled)
{
    if (m_enabled == enabled)
        return;
    m_enabled = enabled;
    enabledChanged(m_enabled);
}

void DiscriminatorProxyModelBase::discriminateAll()
{
    if (!sourceModel())
        return;

    beginResetModel();
    for (auto &data : m_data)
        data.discriminatingCheckState = Qt::Checked;
    endResetModel();
}

void DiscriminatorProxyModelBase::discriminateNone()
{
    if (!sourceModel())
        return;

    beginResetModel();
    for (auto &data : m_data)
        data.discriminatingCheckState = Qt::Unchecked;
    endResetModel();
}

void DiscriminatorProxyModelBase::filterAll()
{
    if (!sourceModel())
        return;

    beginResetModel();
    for (auto &data : m_data) {
        data.filteringCheckState = Qt::Checked;
    }
    endResetModel();
}

void DiscriminatorProxyModelBase::filterNone()
{
    if (!sourceModel())
        return;

    beginResetModel();
    for (auto &data : m_data)
        data.filteringCheckState = Qt::Unchecked;
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

/*****************************************************************************
 * FilterProxyModel
 ****************************************************************************/

#include <3rdparty/kde/kmodelindexproxymapper.h>

#include <QDebug>

FilterProxyModel::FilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent) {}

FilterProxyModel::~FilterProxyModel() = default;

void FilterProxyModel::setDiscriminatorModel(
    DiscriminatorProxyModelBase *model) {

    if (m_discriminator == model)
        return;
    if (m_discriminator) {
        m_discriminator->disconnect(this);
    }
    if (m_indexMapper)
        delete m_indexMapper;

    m_discriminator = model;

    if (m_discriminator) {
        connect(m_discriminator, &QAbstractItemModel::dataChanged, this,
                &QSortFilterProxyModel::invalidate);
        connect(m_discriminator, &QAbstractItemModel::modelReset, this,
                &QSortFilterProxyModel::invalidate);
        connect(m_discriminator, &DiscriminatorProxyModelBase::enabledChanged,
                this, &QSortFilterProxyModel::invalidate);
        qDebug() << Q_FUNC_INFO << "Invalidated";
        if (sourceModel())
            m_indexMapper = new KModelIndexProxyMapper(sourceModel(),
                                                       m_discriminator, this);
    }
}

void GammaRay::FilterProxyModel::setSourceModel(QAbstractItemModel *model) {

    if (sourceModel() == model)
        return;
    if (sourceModel())
        delete m_indexMapper;
    QSortFilterProxyModel::setSourceModel(model);
    if (sourceModel() && m_discriminator)
        m_indexMapper =
            new KModelIndexProxyMapper(sourceModel(), m_discriminator, this);
}

bool FilterProxyModel::filterAcceptsRow(int sourceRow,
                                        const QModelIndex &sourceParent) const {
    Q_ASSERT(m_discriminator);
    Q_ASSERT(sourceModel());
    Q_ASSERT(m_indexMapper);

    qDebug() << Q_FUNC_INFO << sourceRow << sourceParent;
    const auto sourceIndex = sourceModel()->index(sourceRow, 0, sourceParent);
    qDebug() << Q_FUNC_INFO << sourceIndex;
    const auto discriminatorIndex = m_indexMapper->mapLeftToRight(sourceIndex);
    qDebug() << Q_FUNC_INFO << discriminatorIndex;
    const auto discriminatorSourceIndex =
        m_discriminator->mapToSource(discriminatorIndex);
    qDebug() << Q_FUNC_INFO << discriminatorSourceIndex;
    return m_discriminator->isAccepting(discriminatorSourceIndex);
}

QMap<int, QVariant>
GammaRay::FilterProxyModel::itemData(const QModelIndex &index) const {
    return sourceModel()->itemData(mapToSource(index));
}

/*****************************************************************************
 * DiscriminatorBase
 ****************************************************************************/

DiscriminatorBase::DiscriminatorBase(const QString &name, QObject *parent)
    : DiscriminatorInterface(name, parent) {}

DiscriminatorBase::~DiscriminatorBase() = default;

bool DiscriminatorBase::isEnabled() const {
    Q_ASSERT(m_discriminatorProxyModel);
    return m_discriminatorProxyModel->isEnabled();
}

void DiscriminatorBase::setEnabled(bool enabled) {
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
