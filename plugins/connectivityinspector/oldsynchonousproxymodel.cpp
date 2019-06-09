/*
  synchonousproxymodel.cpp

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

#include "synchonousproxymodel.h"

#include <common/remotemodelroles.h>

using namespace GammaRay;

SynchonousProxyModel::SynchonousProxyModel(QObject *parent)
    : QIdentityProxyModel(parent)
{}

void SynchonousProxyModel::addSynchronousRequirement(int column, int role)
{
    if (m_requirements.contains(column) && m_requirements.value(column).contains(role))
        return;
    m_requirements[column].insert(role);
    clear();
}

SynchonousProxyModel::~SynchonousProxyModel() = default;

bool SynchonousProxyModel::isSynchronised() const
{
    return m_isSynchronised;
}

void SynchonousProxyModel::clear()
{
    beginResetModel();
    m_isSynchronised = false;
    m_pendingSourceRows.clear();
    endResetModel();
}

int SynchonousProxyModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return QIdentityProxyModel::rowCount(parent) - m_pendingSourceRows.count();
}

void SynchonousProxyModel::setSourceModel(QAbstractItemModel *sourceModel)
{
    Q_ASSERT(this->sourceModel() == nullptr);
    m_isSynchronised = false;
    QIdentityProxyModel::setSourceModel(sourceModel);
    connect(sourceModel, &QAbstractItemModel::rowsInserted, this, &SynchonousProxyModel::addRows);
    connect(sourceModel, &QAbstractItemModel::rowsRemoved, this, &SynchonousProxyModel::removeRows);
    connect(sourceModel, &QAbstractItemModel::dataChanged, this, &SynchonousProxyModel::updateRows);
}

QModelIndex SynchonousProxyModel::mapToSource(const QModelIndex &proxyIndex) const
{
    Q_ASSERT(proxyIndex.isValid());
    Q_ASSERT(!proxyIndex.parent().isValid());
    const int proxyRow = proxyIndex.row();
    int sourceRow = proxyRow;
    for (const int pendingSourceRow : m_pendingSourceRows) {
        if (pendingSourceRow >= proxyRow)
            sourceRow += 1;
    }
    return sourceModel()->index(sourceRow, proxyIndex.column());
}

QModelIndex SynchonousProxyModel::mapFromSource(const QModelIndex &sourceIndex) const
{
    Q_ASSERT(sourceIndex.isValid());
    Q_ASSERT(!sourceIndex.parent().isValid());
    const int sourceRow = sourceIndex.row();
    int proxyRow = sourceRow;
    for (const int pendingSourceRow : m_pendingSourceRows) {
        if (pendingSourceRow < proxyRow)
            proxyRow -= 1;
    }
    return index(proxyRow, sourceIndex.column());
}

void SynchonousProxyModel::addRows(const QModelIndex &index, int first, int last)
{
    Q_ASSERT(index.isValid());
    Q_ASSERT(!index.parent().isValid());

    if (m_requirements.isEmpty()) {
        beginInsertRows({}, first, last);
        endInsertRows();
        return;
    }
    for (int sourceRow = first; sourceRow <= last; ++sourceRow)
        m_pendingSourceRows.insert(sourceRow);
    m_isSynchronised = false;
}

void SynchonousProxyModel::removeRows(const QModelIndex &index, int first, int last)
{
    Q_ASSERT(index.isValid());
    Q_ASSERT(!index.parent().isValid());

    if (m_requirements.isEmpty())
        return;
    for (int sourceRow = first; sourceRow <= last; ++sourceRow) {
        if (m_pendingSourceRows.contains(sourceRow)) {
            beginRemoveRows({}, proxyRow, proxyRow);
            endRemoveRows();
        }
    }
}

void SynchonousProxyModel::updateRows(const QModelIndex &topLeft,
                                      const QModelIndex &bottomRight,
                                      const QVector<int> & /*roles*/)
{
    if (m_requirements.isEmpty())
        return;

    const int firstSourceRow = topLeft.row();
    const int lastSourceRow = bottomRight.row();
    for (int sourceRow = firstSourceRow; sourceRow <= lastSourceRow; ++sourceRow) {
        if (!m_pendingSourceRows.contains(sourceRow))
            continue;
        for (int column : m_requirements.keys()) {
            const auto sourceIndex = sourceModel()->index(sourceRow, column);
            const auto state = sourceIndex.data(RemoteModelRole::LoadingState)
                                   .value<RemoteModelNodeState::NodeState>();
            if (state == 0) {
                const auto proxyIndex = mapFromSource(sourceIndex);
                const auto proxyRow = proxyIndex.row();
                beginInsertRows({}, proxyRow, proxyRow);
                m_pendingSourceRows.remove(sourceRow);
                endInsertRows();
            }
        }
    }
}

bool GammaRay::SynchonousProxyModel::filterAcceptsRow(int sourceRow,
                                                      const QModelIndex &sourceParent) const
{
    Q_ASSERT(!sourceParent.isValid());
    for (int column : m_requirements.keys()) {
        const auto sourceIndex = sourceModel()->index(sourceRow, column);
        const auto state = sourceIndex.data(RemoteModelRole::LoadingState)
                               .value<RemoteModelNodeState::NodeState>();
        if (state != 0)
            return false;
    }
    return true;
}
