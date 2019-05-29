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
            qWarning() << __FUNCTION__ << srcIndex << data.isRecording;
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

    qWarning() << __FUNCTION__ << parent << row << extraColumn << value << role;
    const auto srcIndex = sourceModel()->index(row, 0, mapToSource(parent));
    RecordingData &data = recordingData(srcIndex);
    const quint64 enabled = value.toInt() == Qt::Checked ? 1 : 0;
    qWarning() << __FUNCTION__ << srcIndex << enabled;
    if (extraColumn == IsRecordingColumn) {
        data.isRecording = enabled;
    } else if (extraColumn == IsVisibleColumn) {
        data.isVisible = enabled;
    } else
        return false;
    const auto change = index(row, extraColumn, parent);
    qWarning() << __FUNCTION__ << change << enabled << data.isRecording
               << recordingData(srcIndex).isRecording;
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

void RecordingProxyModelBase::processItem(const QModelIndex &sourceIndex)
{
    const int rowCount = sourceModel()->rowCount(sourceIndex);
    qWarning() << __FUNCTION__ << sourceIndex << rowCount;
    for (int row = 0; row < rowCount; ++row) {
        const auto index = sourceModel()->index(row, 0, sourceIndex);
        qWarning() << __FUNCTION__ << sourceIndex << row << rowCount;
        addRecordingData(index);
        processItem(index);
    }
}

void RecordingProxyModelBase::initialiseRecordingModel()
{
    processItem(QModelIndex());
    connect(sourceModel(),
            &QAbstractItemModel::rowsInserted,
            this,
            [this](const QModelIndex &parent, int first, int last) {
                for (int row = first; row <= last; ++row) {
                    const auto index = sourceModel()->index(row, 0, parent);
                    qWarning() << __FUNCTION__ << parent << index << row << last;
                    addRecordingData(index);
                }
            });
    connect(sourceModel(),
            &QAbstractItemModel::rowsAboutToBeRemoved,
            this,
            [this](const QModelIndex &parent, int first, int last) {
                for (int row = first; row <= last; ++row) {
                    const auto index = sourceModel()->index(row, 0, parent);
                    qWarning() << __FUNCTION__ << parent << index << row << last;
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

#if 0
RecordingProxyModel::RecordingProxyModel(QObject *parent)
    : KExtraColumnsProxyModel(parent)
{
    appendColumn("Count");
    appendColumn("Record");
    appendColumn("Show");
}

RecordingProxyModel::~RecordingProxyModel() = default;

#if 0
QModelIndex RecordingProxyModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!sourceModel())
        return {};

    if (!parent.isValid())
        return createIndex(row, column);
    return {};
}

QModelIndex RecordingProxyModel::parent(const QModelIndex &child) const
{
    if (!sourceModel())
        return {};
    return mapFromSource(sourceModel()->parent(mapToSource(child)));
}

int RecordingProxyModel::rowCount(const QModelIndex &parent) const
{
    if (!sourceModel())
        return 0;

    return sourceModel()->rowCount(mapToSource(parent));
}
int RecordingProxyModel::columnCount(const QModelIndex &parent) const
{
    if (!sourceModel())
        return 0;

    //if (!parent.isValid())
    return QIdentityProxyModel::columnCount(parent) + ExtraColumnCount;
    //return {};
}

QVariant RecordingProxyModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (!sourceModel())
        return {};

    if (section < sourceModel()->columnCount())
        return QIdentityProxyModel::headerData(section, orientation, role);

    if (role != Qt::DisplayRole)
        return {};

    if (orientation != Qt::Horizontal)
        return {};

    const int extraSection = section - sourceModel()->columnCount();
    switch (extraSection) {
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

QVariant RecordingProxyModel::data(const QModelIndex &index, int role) const
{
    if (!sourceModel())
        return {};

    if (index.column() < sourceModel()->columnCount())
        return QIdentityProxyModel::data(index, role);

    const int row = index.row();
    const int extraColumn = index.column() - sourceModel()->columnCount();
    const RecordingData &data = m_recordingData.at(row);
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

bool RecordingProxyModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!sourceModel())
        return {};

    if (!index.isValid())
        return false;

    if (index.column() < sourceModel()->columnCount())
        return QIdentityProxyModel::setData(index, value, role);

    if (role != Qt::CheckStateRole)
        return false;

    const int row = index.row();
    const int column = index.column() - sourceModel()->columnCount();
    const bool enabled = value.toInt() == Qt::Checked;
    auto &data = m_recordingData[row];
    if (column == IsRecordingColumn) {
        data.isRecording = enabled;
        emit isRecordingChanged();
    } else if (column == IsVisibleColumn) {
        data.isVisible = enabled;
    } else
        return false;
    emit dataChanged(index, index, {Qt::CheckStateRole});
    return true;
}

Qt::ItemFlags RecordingProxyModel::flags(const QModelIndex &index) const
{
    if (!sourceModel())
        return {};

    if (index.column() < sourceModel()->columnCount())
        return QIdentityProxyModel::flags(index);

    if (!index.isValid())
        return Qt::ItemFlag::NoItemFlags;

    const int column = index.column() - sourceModel()->columnCount();
    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    if (column == IsRecordingColumn || column == IsVisibleColumn)
        flags |= Qt::ItemIsUserCheckable;

    return flags;
}
#endif

void RecordingProxyModel::setSourceModel(QAbstractItemModel *model)
{
    if (sourceModel())
        finaliseRecordingModel();
    QIdentityProxyModel::setSourceModel(model);
    if (sourceModel())
        initialiseRecordingModel();
}

#if 0
QModelIndex RecordingProxyModel::mapToSource(const QModelIndex &proxyIndex) const
{
    if (!sourceModel())
        return {};

    if (!proxyIndex.isValid())
        return {};
    if (proxyIndex.column() >= sourceModel()->columnCount())
        return {};
    return QIdentityProxyModel::mapToSource(proxyIndex);
}

QModelIndex RecordingProxyModel::mapFromSource(const QModelIndex &sourceIndex) const
{
    if (!sourceModel())
        return {};

    if (sourceIndex.isValid())
        return createIndex(sourceIndex.row(), sourceIndex.column());
    //    if (sourceIndex.column() >= sourceModel()->columnCount())
    //        return createIndex(sourceIndex.row(), sourceIndex.column() + ExtraColumnCount);
    //    return createIndex(sourceIndex.row(), sourceIndex.column());
    return {};
}
#endif
void RecordingProxyModel::increaseCount(int row)
{
    if (!sourceModel())
        return;

    m_recordingData[row].count++;
    const auto change = index(row, CountColumn, QModelIndex());
    emit dataChanged(change, change);
}

void RecordingProxyModel::decreaseCount(int row)
{
    if (!sourceModel())
        return;

    m_recordingData[row].count--;
    const auto change = index(row, CountColumn, QModelIndex());
    emit dataChanged(change, change);
}

void RecordingProxyModel::resetCounts()
{
    if (!sourceModel())
        return;

    beginResetModel();
    for (int row = 0; row < rowCount(QModelIndex()); ++row)
        m_recordingData[row].count = 0;
    endResetModel();
}

bool RecordingProxyModel::isRecording(int row) const
{
    if (!sourceModel())
        return false;

    return m_recordingData[row].isRecording;
}

void RecordingProxyModel::recordAll()
{
    if (!sourceModel())
        return;

    beginResetModel();
    for (int row = 0; row < rowCount(QModelIndex()); ++row)
        m_recordingData[row].isRecording = true;
    endResetModel();
}

void RecordingProxyModel::recordNone()
{
    if (!sourceModel())
        return;

    beginResetModel();
    for (int row = 0; row < rowCount(QModelIndex()); ++row)
        m_recordingData[row].isRecording = false;
    endResetModel();
}

bool RecordingProxyModel::isVisible(int row) const
{
    if (!sourceModel())
        return false;

    return m_recordingData[row].isVisible;
}

void RecordingProxyModel::showAll()
{
    if (!sourceModel())
        return;

    beginResetModel();
    for (int row = 0; row < rowCount(QModelIndex()); ++row)
        m_recordingData[row].isVisible = true;
    endResetModel();
}

void RecordingProxyModel::showNone()
{
    if (!sourceModel())
        return;

    beginResetModel();
    for (int row = 0; row < rowCount(QModelIndex()); ++row)
        m_recordingData[row].isVisible = false;
    endResetModel();
}

void RecordingProxyModel::processItem(const QModelIndex &sourceIndex)
{
    const int rowCount = sourceModel()->rowCount(sourceIndex);
    for (int row = 0; row < rowCount; ++row) {
        const auto childIndex = sourceModel()->index(row, 0, sourceIndex);
        m_data.insert(QPersistentModelIndex(childIndex), RecordingData());
        if (childIndex.isValid())
            processItem(childIndex);
    }
}

void RecordingProxyModel::initialiseRecordingModel()
{
    processItem(QModelIndex());
    connect(sourceModel(),
            &QAbstractItemModel::rowsAboutToBeInserted,
            this,
            &RecordingProxyModel::insertRecordingRows);
    connect(sourceModel(),
            &QAbstractItemModel::rowsRemoved,
            this,
            &RecordingProxyModel::removeRecordingRows);
    connect(sourceModel(),
            &QAbstractItemModel::modelReset,
            this,
            &RecordingProxyModel::resetRecordingModel);
}

void RecordingProxyModel::finaliseRecordingModel()
{
    m_data.clear();
    sourceModel()->disconnect(this);
}

void RecordingProxyModel::insertRecordingRows(const QModelIndex &parent, int first, int last)
{
    if (!sourceModel())
        return;
    for (int row = first; row <= last; ++row)
        m_data.insert(sourceModel()->index(row, 0, parent), RecordingData());
}

void RecordingProxyModel::removeRecordingRows(const QModelIndex &parent, int first, int last)
{
    if (!sourceModel())
        return;

    for (int row = first; row <= last; ++row)
        m_data.remove(sourceModel()->index(row, 0, parent));
}

void RecordingProxyModel::resetRecordingModel()
{
    if (!sourceModel())
        return;

    m_data.clear();
}

QVariant GammaRay::RecordingProxyModel::extraColumnData(const QModelIndex &parent,
                                                        int row,
                                                        int extraColumn,
                                                        int role) const
{
    if (!sourceModel())
        return {};

    const RecordingData &data = m_data[parent.internalId()];
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

bool GammaRay::RecordingProxyModel::setExtraColumnData(
    const QModelIndex &parent, int row, int extraColumn, const QVariant &data, int role)
{}
#endif
