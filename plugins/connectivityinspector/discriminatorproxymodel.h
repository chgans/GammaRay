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

#include "discriminatorinterface.h"

namespace GammaRay {

// From Chapter 1 of http://shop.oreilly.com/product/9780596514556.do:
//  - Acquire: Obtain the data
//  - Parse: Organise data into meaninful, categorised structures
//  - Filter: Remove all but the data of interest
//  - Mine: Discern patterns, use stats, math, algortihms, ...
//  - Represent: Basic visualisation model
//  - Refine: Improve representation: clearer and visually engaging
//  - Interact: Allow data manipulation, feature visibility
//
// TODO: Improve/fix filtering in AcquisitionEngine
//
//               +-------------------------------- DiscriminatorIface ---+
//               v                                                       |
//         +-> DiscriminatorProxy ---------------> ServerProxy ------> ClientFilterGui
//         |     ^
// Source -+     |
//         |     v
//         +-> FilterProxy -> AcquisitionEngine -> ServerProxy ------> ClientGraphGui
//                              ^
//                              +----------------- AcquisitionIface -- ClientAcquisitionGui
//
// Tasks:
//  - Rename FilterProxyModel to DiscriminatorProxyModel
//  - Rename {record|show}{All|None} to {include|exclude}{All|None}
//  - Rename is{Recording|Visible} to is{Including|Excluding}
//  - Create a real FilterProxyModel that filters Source by querying Discriminator
//  - The AcquisitionEngine should output filtered MetaObject, Object, Thread lists and an Connection list
//  - The gui should consume the AcquisitionEngine output, and deal with isVisible itself

class DiscriminatorProxyModelBase : public KExtraColumnsProxyModel
{
    Q_OBJECT

public:
    DiscriminatorProxyModelBase(QObject *parent = nullptr);
    ~DiscriminatorProxyModelBase() override;

    enum ExtraColumn { CountColumn = 0, IsDiscrimatingColumn, IsFilteringColumn, ExtraColumnCount };

    void setEnabled(bool enabled);
    void discriminateAll();
    void discriminateNone();
    void filterAll();
    void filterNone();
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

    Qt::ItemFlags extraColumnFlags(const QModelIndex &parent,
                                   int row,
                                   int extraColumn) const override;

    virtual QMap<int, QVariant>
    extraItemData(const QModelIndex &index) const override;

protected:
    struct ItemData
    {
        ItemData()
            : count(0)
            , isDiscriminating(1)
            , isFiltering(1)
        {}
        quint64 count : 62;
        quint64 isDiscriminating : 1;
        quint64 isFiltering : 1;
    };

    void increaseUsageCount(const QModelIndex &index);
    void decreaseUsageCount(const QModelIndex &index);
    quint64 usageCount(const QModelIndex &index) const;
    bool isDiscriminating(const QModelIndex &index) const;
    void setIsDiscriminating(const QModelIndex &index, bool enabled);
    bool isFiltering(const QModelIndex &index) const;
    void setIsFiltering(const QModelIndex &index, bool enabled);
    bool isAccepting(const QModelIndex &index) const
    {
        return !(m_enabled && isDiscriminating(index) && isFiltering(index));
    }

    virtual void addItemData(const QModelIndex &index) = 0;
    virtual void removeItemData(const QModelIndex &index) = 0;
    virtual void clearItemData() = 0;

private:
    using IndexVisitor = std::function<void(const QModelIndex &)>;
    QHash<QPersistentModelIndex, ItemData> m_data;
    quint64 m_maxCount = 0;
    bool m_enabled = false;

private slots:
    void visitIndex(const QModelIndex &sourceIndex, IndexVisitor visitor);
    void initialiseDiscriminator();
    void finaliseDiscriminator();
};

template<typename T, int TargetRole>
class DiscriminatorProxyModel : public DiscriminatorProxyModelBase
{
public:
    DiscriminatorProxyModel(QObject *parent = nullptr)
        : DiscriminatorProxyModelBase(parent)
    {}
    ~DiscriminatorProxyModel() override = default;

    void increaseUsageCount(const T &target)
    {
        if (!sourceModel())
            return;

        Q_ASSERT(m_indexes.contains(target));
        DiscriminatorProxyModelBase::increaseUsageCount(m_indexes.value(target));
    }

    void decreaseUsageCount(const T &target)
    {
        if (!sourceModel())
            return;

        Q_ASSERT(m_indexes.contains(target));
        DiscriminatorProxyModelBase::decreaseUsageCount(m_indexes.value(target));
    }

    quint64 usageCount(const T &target)
    {
        if (!sourceModel())
            return 0;

        Q_ASSERT(m_indexes.contains(target));
        return DiscriminatorProxyModelBase::usageCount(m_indexes.value(target));
    }

    bool isDiscriminating(const T &target) const
    {
        if (!sourceModel())
            return false;
        if (!m_indexes.contains(target))
            return false; // FIXME: from main update
        // FIXME: inheritance?
        Q_ASSERT(m_indexes.contains(target));
        return DiscriminatorProxyModelBase::isDiscriminating(m_indexes.value(target));
    }

    bool isFiltering(const T &target) const
    {
        if (!sourceModel())
            return false;

        if (!m_indexes.contains(target))
            return false; // FIXME:  from main update
        // FIXME: inheritance?
        Q_ASSERT(m_indexes.contains(target));
        return DiscriminatorProxyModelBase::isFiltering(m_indexes.value(target));
    }

    bool isAccepting(const T &target) const
    {
        if (!sourceModel())
            return false;

        if (!m_indexes.contains(target))
            return false; // FIXME:  from main update
        Q_ASSERT(m_indexes.contains(target));
        return DiscriminatorProxyModelBase::isAccepting(m_indexes.value(target));
    }

    // TODO: something more efficient
    QList<T> targets() const { return m_indexes.keys(); }

    // RecordingProxyModelBase interface
protected:
    void addItemData(const QModelIndex &index) override
    {
        auto target = index.data(TargetRole).value<T>();
        // FIXME: not sure it's a one to one relationship here, due to invalid
        // objects...
        m_indexes.insert(target, index);
    }

    void removeItemData(const QModelIndex &index) override
    {
        auto target = index.data(TargetRole).value<T>();
        // FIXME:
        // Q_ASSERT(m_indexes.contains(target));
        m_indexes.remove(target);
    }

    void clearItemData() override { m_indexes.clear(); }

private:
    // TODO: provide iterator/view interface?
    QHash<T, QPersistentModelIndex> m_indexes;
};

class DiscriminatorInterfaceBridge : public DiscriminatorInterface
{
    Q_OBJECT
public:
    DiscriminatorInterfaceBridge(const QString &name,
                                 DiscriminatorProxyModelBase *model,
                                 QObject *parent = nullptr)
        : DiscriminatorInterface(name, parent)
        , m_model(model)
    {}
    ~DiscriminatorInterfaceBridge() override = default;

    // DiscriminatorInterface interface
public slots:
    void setEnabled(bool enabled) override { m_model->setEnabled(enabled); };
    void discriminateAll() override { m_model->discriminateAll(); }
    void discriminateNone() override { m_model->discriminateNone(); }
    void filterAll() override { m_model->filterAll(); }
    void filterNone() override { m_model->filterNone(); }

private:
    DiscriminatorProxyModelBase *m_model;
};

} // namespace GammaRay

#endif // GAMMARAY_CONNECTIVITYINSPECTOR_RECORDINGPROXYMODEL_H
