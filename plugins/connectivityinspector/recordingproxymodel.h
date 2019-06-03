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

#ifndef GAMMARAY_CONNECTIVITYINSPECTOR_RECORDINGPROXYMODEL_H
#define GAMMARAY_CONNECTIVITYINSPECTOR_RECORDINGPROXYMODEL_H

#include "3rdparty/kde/kextracolumnsproxymodel.h"

#include "recordinginterface.h"

namespace GammaRay {

class RecordingProxyModelBase : public KExtraColumnsProxyModel
{
    Q_OBJECT

public:
    RecordingProxyModelBase(QObject *parent = nullptr);
    ~RecordingProxyModelBase() override;

    enum ExtraColumn { CountColumn = 0, IsRecordingColumn, IsVisibleColumn, ExtraColumnCount };

    void recordAll();
    void recordNone();
    void showAll();
    void showNone();

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
    virtual void setIsRecording(const QModelIndex &index, bool enabled) = 0;
    virtual void setIsVisible(const QModelIndex &index, bool enabled) = 0;

private:
    using IndexVisitor = std::function<void(const QModelIndex &)>;
private slots:
    void visitIndex(const QModelIndex &sourceIndex, IndexVisitor visitor);
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

    bool isVisible(const T &target) const
    {
        if (!sourceModel())
            return false;

        Q_ASSERT(m_indexes.contains(target));
        return m_data[m_indexes.value(target)].isVisible;
    }

    // RecordingProxyModelBase interface
protected:
    void addRecordingData(const QModelIndex &index) override
    {
        auto target = index.data(TargetRole).value<T>();
        Q_ASSERT(!m_data.contains(index));
        m_data.insert(index, {});
        m_indexes.insert(target, index);
    }

    void removeRecordingData(const QModelIndex &index) override
    {
        auto target = index.data(TargetRole).value<T>();
        Q_ASSERT(m_data.contains(index));
        m_data.remove(index);
        m_indexes.remove(target);
    }

    void clearRecordingData() override
    {
        m_indexes = QHash<T, QPersistentModelIndex>();
        m_data = QHash<QPersistentModelIndex, RecordingData>();
    }

    RecordingData &recordingData(const QModelIndex &index) override
    {
        Q_ASSERT(m_data.contains(index));
        return m_data[index];
    }

    RecordingData recordingData(const QModelIndex &index) const override
    {
        Q_ASSERT(m_data.contains(index));
        return m_data.value(index);
    }

    void setIsRecording(const QModelIndex &index, bool enabled) override
    {
        Q_ASSERT(m_data.contains(index));
        m_data[index].isRecording = enabled;
    }

    void setIsVisible(const QModelIndex &index, bool enabled) override
    {
        Q_ASSERT(m_data.contains(index));
        m_data[index].isVisible = enabled;
    }

private:
    QHash<QPersistentModelIndex, RecordingData> m_data;
    QHash<T, QPersistentModelIndex> m_indexes;
};

class RecordingInterfaceBridge : public ConnectivityRecordingInterface
{
    Q_OBJECT
public:
    RecordingInterfaceBridge(const QString &name,
                             RecordingProxyModelBase *model,
                             QObject *parent = nullptr)
        : ConnectivityRecordingInterface(name, parent)
        , m_model(model)
    {}
    ~RecordingInterfaceBridge() override = default;

    // ConnectivityInspectorRecordingInterface interface
public slots:
    void recordAll() override { m_model->recordAll(); }
    void recordNone() override { m_model->recordNone(); }
    void showAll() override { m_model->showAll(); }
    void showNone() override { m_model->showNone(); }

private:
    RecordingProxyModelBase *m_model;
};

} // namespace GammaRay

#endif // GAMMARAY_CONNECTIVITYINSPECTOR_RECORDINGPROXYMODEL_H
