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

#include "filterinterface.h"

namespace GammaRay {

class FilterProxyModelBase : public KExtraColumnsProxyModel {
    Q_OBJECT

public:
    FilterProxyModelBase(QObject *parent = nullptr);
    ~FilterProxyModelBase() override;

    enum ExtraColumn { CountColumn = 0, IsRecordingColumn, IsVisibleColumn, ExtraColumnCount };

    void recordAll();
    void recordNone();
    void showAll();
    void showNone();
    void resetCounts();

    // QAbstractProxyModel interface
public:
    void setSourceModel(QAbstractItemModel *model) override;

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

    virtual QMap<int, QVariant>
    extraItemData(const QModelIndex &index) const override;

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

    void increaseCount(const QModelIndex &index);
    void decreaseCount(const QModelIndex &index);
    quint64 recordCount(const QModelIndex &index) const;
    bool isRecording(const QModelIndex &index) const;
    void setIsRecording(const QModelIndex &index, bool enabled);
    bool isVisible(const QModelIndex &index) const;
    void setIsVisible(const QModelIndex &index, bool enabled);

    virtual void addRecordingData(const QModelIndex &index) = 0;
    virtual void removeRecordingData(const QModelIndex &index) = 0;
    virtual void clearRecordingData() = 0;

private:
    using IndexVisitor = std::function<void(const QModelIndex &)>;
    QHash<QPersistentModelIndex, RecordingData> m_data;
    quint64 m_maxCount = 0;

private slots:
    void visitIndex(const QModelIndex &sourceIndex, IndexVisitor visitor);
    void initialiseRecordingModel();
    void finaliseRecordingModel();
};

template <typename T, int TargetRole>
class FilterProxyModel : public FilterProxyModelBase {
public:
    FilterProxyModel(QObject *parent = nullptr)
        : FilterProxyModelBase(parent) {}
    ~FilterProxyModel() override = default;

    void increaseCount(const T &target)
    {
        if (!sourceModel())
            return;

        Q_ASSERT(m_indexes.contains(target));
        FilterProxyModelBase::increaseCount(m_indexes.value(target));
    }

    void decreaseCount(const T &target)
    {
        if (!sourceModel())
            return;

        Q_ASSERT(m_indexes.contains(target));
        const auto &srcIndex = m_indexes[target];
        FilterProxyModelBase::decreaseCount(m_indexes.value(target));
    }

    quint64 recordCount(const T &target) {
        if (!sourceModel())
            return 0;

        Q_ASSERT(m_indexes.contains(target));
        const auto &srcIndex = m_indexes[target];
        return FilterProxyModelBase::recordCount(m_indexes.value(target));
    }

    bool isRecording(const T &target) const
    {
        if (!sourceModel())
            return false;
        if (!m_indexes.contains(target))
            return false; // FIXME: from main update
        // FIXME: inheritance?
        Q_ASSERT(m_indexes.contains(target));
        return FilterProxyModelBase::isRecording(m_indexes.value(target));
    }

    bool isVisible(const T &target) const
    {
        if (!sourceModel())
            return false;

        if (!m_indexes.contains(target))
            return false; // FIXME:  from main update
        // FIXME: inheritance?
        Q_ASSERT(m_indexes.contains(target));
        return FilterProxyModelBase::isVisible(m_indexes.value(target));
    }

    // TODO: something more efficient
    QList<T> targets() const { return m_indexes.keys(); }

    // RecordingProxyModelBase interface
protected:
    void addRecordingData(const QModelIndex &index) override {
        auto target = index.data(TargetRole).value<T>();
        // FIXME: not sure it's a one to one relationship here, due to invalid
        // objects...
        m_indexes.insert(target, index);
    }

    void removeRecordingData(const QModelIndex &index) override {
        auto target = index.data(TargetRole).value<T>();
        // FIXME:
        // Q_ASSERT(m_indexes.contains(target));
        m_indexes.remove(target);
    }

    void clearRecordingData() override { m_indexes.clear(); }

    // TODO: provide iterator/view interface?

private:
    QHash<T, QPersistentModelIndex> m_indexes;
};

class RecordingInterfaceBridge : public FilterInterface {
    Q_OBJECT
public:
    RecordingInterfaceBridge(const QString &name, FilterProxyModelBase *model,
                             QObject *parent = nullptr)
        : FilterInterface(name, parent), m_model(model) {}
    ~RecordingInterfaceBridge() override = default;

    // ConnectivityRecordingInterface interface
public slots:
    void recordAll() override { m_model->recordAll(); }
    void recordNone() override { m_model->recordNone(); }
    void showAll() override { m_model->showAll(); }
    void showNone() override { m_model->showNone(); }

private:
    FilterProxyModelBase *m_model;
};

} // namespace GammaRay

#endif // GAMMARAY_CONNECTIVITYINSPECTOR_RECORDINGPROXYMODEL_H
