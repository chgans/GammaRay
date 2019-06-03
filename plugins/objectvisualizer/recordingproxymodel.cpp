/*
  recordingproxymodel.cpp

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

#include "recordingproxymodel.h"

#include <functional>

#include <QDebug>

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
    auto srcIndex = sourceModel()->index(row, 0, mapToSource(parent));
    const auto data = recordingData(srcIndex);
    if (role == Qt::DisplayRole) {
        switch (extraColumn) {
        case CountColumn:
            return data.count;
        }
    } else if (role == Qt::CheckStateRole) {
        switch (extraColumn) {
        case IsRecordingColumn:
            return data.isRecording ? Qt::Checked : Qt::Unchecked;
        case IsVisibleColumn:
            return data.isVisible ? Qt::Checked : Qt::Unchecked;
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
    RecordingData &data = recordingData(srcIndex);
    const quint64 enabled = value.toInt() == Qt::Checked ? 1 : 0;
    if (extraColumn == IsRecordingColumn) {
        data.isRecording = enabled;
    } else if (extraColumn == IsVisibleColumn) {
        data.isVisible = enabled;
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

void RecordingProxyModelBase::recordAll()
{
    if (!sourceModel())
        return;

    beginResetModel();
    visitIndex(QModelIndex(), [this](const QModelIndex &index) { setIsRecording(index, true); });
    endResetModel();
}

void RecordingProxyModelBase::recordNone()
{
    if (!sourceModel())
        return;

    beginResetModel();
    visitIndex(QModelIndex(), [this](const QModelIndex &index) { setIsRecording(index, false); });
    endResetModel();
}

void RecordingProxyModelBase::showAll()
{
    if (!sourceModel())
        return;

    beginResetModel();
    visitIndex(QModelIndex(), [this](const QModelIndex &index) { setIsVisible(index, true); });
    endResetModel();
}

void RecordingProxyModelBase::showNone()
{
    if (!sourceModel())
        return;

    beginResetModel();
    visitIndex(QModelIndex(), [this](const QModelIndex &index) { setIsVisible(index, false); });
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
    visitIndex(QModelIndex(), [this](const QModelIndex &index) { addRecordingData(index); });
    connect(sourceModel(),
            &QAbstractItemModel::rowsInserted,
            this,
            [this](const QModelIndex &parent, int first, int last) {
                for (int row = first; row <= last; ++row) {
                    const auto index = sourceModel()->index(row, 0, parent);
                    addRecordingData(index);
                }
            });
    connect(sourceModel(),
            &QAbstractItemModel::rowsAboutToBeRemoved,
            this,
            [this](const QModelIndex &parent, int first, int last) {
                for (int row = first; row <= last; ++row) {
                    const auto index = sourceModel()->index(row, 0, parent);
                    removeRecordingData(index);
                }
            });
    //    connect(sourceModel(),
    //            &QAbstractItemModel::modelReset,
    //            this,
    //            &RecordingProxyModelBase::clearRecordingData);
}

void RecordingProxyModelBase::finaliseRecordingModel()
{
    sourceModel()->disconnect(this);
    clearRecordingData();
}
