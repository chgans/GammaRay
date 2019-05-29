/*
  recordingproxymodel.h

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

#ifndef RECORDINGPROXYMODEL_H
#define RECORDINGPROXYMODEL_H

#include "3rdparty/kde/kextracolumnsproxymodel.h"

#include <QDebug>

namespace GammaRay {

class RecordingProxyModelBase : public KExtraColumnsProxyModel
{
    Q_OBJECT

public:
    RecordingProxyModelBase(QObject *parent = nullptr);
    ~RecordingProxyModelBase() override;

    enum ExtraColumn { CountColumn = 0, IsRecordingColumn, IsVisibleColumn, ExtraColumnCount };

    // KExtraColumnsProxyModel interface
public:
    QVariant extraColumnData(const QModelIndex &parent,
                             int row,
                             int extraColumn,
                             int role) const override;

    bool setExtraColumnData(const QModelIndex &parent,
                            int row,
                            int extraColumn,
                            const QVariant &value,
                            int role) override;

    Qt::ItemFlags extraColumnFlags(int extraColumn) const override;

    // QAbstractProxyModel interface
public:
    void setSourceModel(QAbstractItemModel *model) override;

protected:
    struct RecordingData
    {
        RecordingData()
            : count(0)
            , isRecording(0)
            , isVisible(0)
        {}
        quint64 count : 62;
        quint64 isRecording : 1;
        quint64 isVisible : 1;
    };

    virtual RecordingData &recordingData(const QModelIndex &index) = 0;
    virtual RecordingData recordingData(const QModelIndex &index) const = 0;
    virtual void addRecordingData(const QModelIndex &index) = 0;
    virtual void removeRecordingData(const QModelIndex &index) = 0;
    virtual void clearRecordingData() = 0;

private slots:
    void processItem(const QModelIndex &sourceIndex);
    void initialiseRecordingModel();
    void finaliseRecordingModel();
};

template<typename T, int TargetRole>
class RecordingProxyModel : public RecordingProxyModelBase
{
public:
    RecordingProxyModel(QObject *parent = nullptr)
        : RecordingProxyModelBase(parent)
    {}
    ~RecordingProxyModel() override = default;

    void increaseCount(const T &target)
    {
        if (!sourceModel())
            return;

        Q_ASSERT(m_indexes.contains(target));
        const auto &srcIndex = m_indexes[target];
        m_data[srcIndex].count++;
        const auto proxyIndex = mapFromSource(srcIndex);
        emit dataChanged(proxyIndex, proxyIndex);
    }

    void decreaseCount(const T &target)
    {
        if (!sourceModel())
            return;

        Q_ASSERT(m_indexes.contains(target));
        const auto &srcIndex = m_indexes[target];
        m_data[srcIndex].count--;
        const auto proxyIndex = mapFromSource(srcIndex);
        emit dataChanged(proxyIndex, proxyIndex);
    }

    void resetCounts()
    {
        if (!sourceModel())
            return;

        beginResetModel();
        for (auto &data : m_data)
            data.count = 0;
        endResetModel();
    }

    bool isRecording(const T &target) const
    {
        if (!sourceModel())
            return false;

        Q_ASSERT(m_indexes.contains(target));
        return m_data[m_indexes.value(target)].isRecording;
    }

    void recordAll()
    {
        if (!sourceModel())
            return;

        beginResetModel();
        for (auto &data : m_data)
            data.isRecording = true;
        endResetModel();
    }

    void recordNone()
    {
        if (!sourceModel())
            return;

        beginResetModel();
        for (auto &data : m_data)
            data.isRecording = false;
        endResetModel();
    }

    bool isVisible(const T &target) const
    {
        if (!sourceModel())
            return false;

        Q_ASSERT(m_indexes.contains(target));
        return m_data[m_indexes.value(target)].isVisible;
    }

    void showAll()
    {
        if (!sourceModel())
            return;

        beginResetModel();
        for (auto &data : m_data)
            data.isVisible = true;
        endResetModel();
    }

    void showNone()
    {
        if (!sourceModel())
            return;

        beginResetModel();
        for (auto &data : m_data)
            data.isVisible = false;
        endResetModel();
    }

protected:
    void addRecordingData(const QModelIndex &index) override
    {
        auto target = index.data(TargetRole).value<T>();
        qWarning() << __FUNCTION__ << index.data(TargetRole) << target;
        Q_ASSERT(!m_data.contains(index));
        m_data.insert(index, {});
        m_indexes.insert(target, index);
        qWarning() << __FUNCTION__ << index << target;
    }

    void removeRecordingData(const QModelIndex &index) override
    {
        auto target = index.data(TargetRole).value<T>();
        Q_ASSERT(m_data.contains(index));
        m_data.remove(index);
        m_indexes.remove(target);
        qWarning() << __FUNCTION__ << index << target;
    }

    void clearRecordingData() override
    {
        m_indexes = QHash<T, QPersistentModelIndex>();
        m_data = QHash<QPersistentModelIndex, RecordingData>();
    }

    RecordingData &recordingData(const QModelIndex &index) override
    {
        Q_ASSERT(m_data.contains(index));
        qWarning() << __FUNCTION__ << index << m_data[index].isRecording;
        return m_data[index];
    }

    RecordingData recordingData(const QModelIndex &index) const override
    {
        Q_ASSERT(m_data.contains(index));
        return m_data.value(index);
    }

private:
    QHash<QPersistentModelIndex, RecordingData> m_data;
    QHash<T, QPersistentModelIndex> m_indexes;
};
} // namespace GammaRay

#endif // RECORDINGPROXYMODEL_H
