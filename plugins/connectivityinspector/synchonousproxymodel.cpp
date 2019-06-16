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

#include <QDebug>

using namespace GammaRay;

SynchonousProxyModel::SynchonousProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{}

void SynchonousProxyModel::addSynchronousRequirement(int column, int role)
{
    if (m_requirements.contains(column) && m_requirements.value(column).contains(role))
        return;
    m_requirements[column].insert(role);
    invalidate();
}

SynchonousProxyModel::~SynchonousProxyModel() = default;

void SynchonousProxyModel::setSourceModel(QAbstractItemModel *model)
{
    if (sourceModel())
        sourceModel()->disconnect(this);

    QSortFilterProxyModel::setSourceModel(model);

    // trigger fetching required data now
    if (sourceModel()->rowCount() > 0)
        fetchAllRequiredData();
    // and later
    connect(model,
            &QAbstractItemModel::rowsInserted,
            this,
            [this](const QModelIndex &parent, int first, int last) {
                fetchRequiredData(parent, first, last);
            });
    //connect(model, &QAbstractItemModel::dataChanged, this, [this]() { fetchAllRequiredData(); });
    connect(model, &QAbstractItemModel::modelReset, this, [this]() { fetchAllRequiredData(); });
}

bool SynchonousProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    bool accepted = true;
    for (int column : m_requirements.keys()) {
        const auto sourceIndex = sourceModel()->index(sourceRow, column, sourceParent);
        const auto state = sourceIndex.data(RemoteModelRole::LoadingState)
                               .value<RemoteModelNodeState::NodeStates>();
        if (state != 0) {
            accepted = false;
        }
    }
    return accepted;
}

void SynchonousProxyModel::fetchRequiredData(const QModelIndex &parent,
                                             int first,
                                             int last,
                                             bool recurse) const
{
    for (int row = first; row < last; ++row) {
        for (int column : m_requirements.keys())
            for (int role : m_requirements.value(column)) {
                const auto index = sourceModel()->index(row, column, parent);
                index.data(role);
                index.flags();
            }
        const auto index = sourceModel()->index(row, 0, parent);
        const auto rowCount = sourceModel()->rowCount(index);
        if (recurse && rowCount) {
            fetchRequiredData(index, 0, rowCount, recurse);
        }
    }
}

void SynchonousProxyModel::fetchAllRequiredData() const
{
    fetchRequiredData({}, 0, sourceModel()->rowCount() - 1, true);
}
