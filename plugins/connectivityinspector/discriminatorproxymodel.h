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

#include "discriminatorinterface.h"

#include "connectivityinspectorcommon.h"

#include <3rdparty/kde/kextracolumnsproxymodel.h>
#include <common/objectmodel.h>
#include <core/probe.h>
#include <core/remote/serverproxymodel.h>

#include <QSortFilterProxyModel>

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
//
//               +-------------------------------- DiscriminatorIface ---+
//               v                                                       |
//             +--------------------+
//         +-> | DiscriminatorProxy |---------------> ServerProxy ------> ClientFilterGui
//         |   |     ^              |
// Source -+   |     |              |
//         |   |     v              |
//         +-> | FilterProxy        |-> AcquisitionEngine -> ServerProxy ------> ClientGraphGui
//             +--------------------+    ^
//                                       +----------------- AcquisitionIface -- ClientAcquisitionGui
//

class DiscriminatorProxyModelBase : public KExtraColumnsProxyModel
{
    Q_OBJECT

public:
    DiscriminatorProxyModelBase(QObject *parent = nullptr);
    ~DiscriminatorProxyModelBase() override;

    enum ExtraColumn { CountColumn = 0, IsDiscrimatingColumn, IsFilteringColumn, ExtraColumnCount };

    void setDiscriminationRole(int role) { m_role = role; }

    void setEnabled(bool enabled);
    void discriminateAll();
    void discriminateNone();
    void filterAll();
    void filterNone();
    void resetCounts();

    void increaseConnectivity(const QModelIndex &index);
    void decreaseConnectivity(const QModelIndex &index);
    bool isAccepting(const QModelIndex &index) const
    {
        return !(m_enabled && isDiscriminating(index) && isFiltering(index));
    }

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
    int m_role = 0;

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

    bool isDiscriminating(const QModelIndex &index) const;
    bool isFiltering(const QModelIndex &index) const;
    virtual void addItemData(const QModelIndex &index) = 0;
    virtual void removeItemData(const QModelIndex &index) = 0;
    virtual void clearItemData() = 0;

private:
    using IndexVisitor = std::function<void(const QModelIndex &)>;
    QHash<QPersistentModelIndex, ItemData> m_data;
    quint64 m_maxCount = 0;
    bool m_enabled = false;

    quint64 connectivityCount(const QModelIndex &index) const;
    void setIsDiscriminating(const QModelIndex &index, bool enabled);
    void setIsFiltering(const QModelIndex &index, bool enabled);

private slots:
    void visitIndex(const QModelIndex &sourceIndex, IndexVisitor visitor);
    void initialiseDiscriminator();
    void finaliseDiscriminator();
};

template<typename T>
class DiscriminatorProxyModel : public DiscriminatorProxyModelBase
{
public:
    DiscriminatorProxyModel(QObject *parent = nullptr)
        : DiscriminatorProxyModelBase(parent)
    {}
    ~DiscriminatorProxyModel() override = default;

    void increaseConnectivity(const T &target)
    {
        if (!sourceModel())
            return;

        Q_ASSERT(m_indexes.contains(target));
        DiscriminatorProxyModelBase::increaseConnectivity(m_indexes.value(target));
    }

    void decreaseConnectivity(const T &target)
    {
        if (!sourceModel())
            return;

        Q_ASSERT(m_indexes.contains(target));
        DiscriminatorProxyModelBase::decreaseConnectivity(m_indexes.value(target));
    }

    quint64 connectivityCount(const T &target)
    {
        if (!sourceModel())
            return 0;

        Q_ASSERT(m_indexes.contains(target));
        return DiscriminatorProxyModelBase::connectivityCount(m_indexes.value(target));
    }

    bool isDiscriminating(const T &target) const
    {
        if (!sourceModel())
            return false;
        Q_ASSERT(m_indexes.contains(target));
        return DiscriminatorProxyModelBase::isDiscriminating(m_indexes.value(target));
    }

    bool isFiltering(const T &target) const
    {
        if (!sourceModel())
            return false;
        Q_ASSERT(m_indexes.contains(target));
        return DiscriminatorProxyModelBase::isFiltering(m_indexes.value(target));
    }

    bool isAccepting(const T &target) const
    {
        if (!sourceModel())
            return false;
        Q_ASSERT(m_indexes.contains(target));
        return DiscriminatorProxyModelBase::isAccepting(m_indexes.value(target));
    }

    // TODO: something more efficient
    QList<T> targets() const { return m_indexes.keys(); }

    // RecordingProxyModelBase interface
protected:
    void addItemData(const QModelIndex &index) override
    {
        auto target = index.data(m_role).template value<T>();
        // FIXME: not sure it's a one to one relationship here, due to invalid
        // objects...
        m_indexes.insert(target, index);
    }

    void removeItemData(const QModelIndex &index) override
    {
        auto target = index.data(m_role).template value<T>();
        // FIXME:
        // Q_ASSERT(m_indexes.contains(target));
        m_indexes.remove(target);
    }

    void clearItemData() override { m_indexes.clear(); }

private:
    // TODO: provide iterator/view interface?
    QHash<T, QPersistentModelIndex> m_indexes;
    int role = 0;
};

class FilterProxyModel : public QSortFilterProxyModel
{
public:
    FilterProxyModel(QObject *parent = nullptr);
    ~FilterProxyModel();

    void setDiscriminatorModel(DiscriminatorProxyModelBase *model);

    // QAbstractItemModel interface
public:
    QMap<int, QVariant> itemData(const QModelIndex &index) const override;

    // QSortFilterProxyModel interface
protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

private:
    DiscriminatorProxyModelBase *m_discriminator = nullptr;
};

class DiscriminatorBase : public DiscriminatorInterface
{
    Q_OBJECT
public:
    DiscriminatorBase(const QString &name, QObject *parent = nullptr)
        : DiscriminatorInterface(name, parent)
    {}
    ~DiscriminatorBase() override = default;

    // DiscriminatorInterface interface
public slots:
    void setEnabled(bool enabled) override;
    void discriminateAll() override;
    void discriminateNone() override;
    void filterAll() override;
    void filterNone() override;

    //    void increaseConnectivity(const QModelIndex &sourceIndex);
    //    void decreaseConnectivity(const QModelIndex &sourceIndex);
    //    bool isAccepting(const QModelIndex &sourceIndex) const;

    const QAbstractItemModel *filteredModel() const;

protected:
    QAbstractItemModel *m_sourceModel = nullptr;
    DiscriminatorProxyModelBase *m_discriminatorProxyModel = nullptr;
    QAbstractProxyModel *m_discriminatorServerProxyModel = nullptr;
    FilterProxyModel *m_filterProxyModel = nullptr;
    QAbstractProxyModel *m_filterServerProxyModel = nullptr;
};

template<class T, class B>
class Discriminator : public DiscriminatorBase
{
public:
    Discriminator(const QString &name, QObject *parent = nullptr)
        : DiscriminatorBase(name, parent)
    {
        m_discriminatorProxyModel = new DiscriminatorProxyModel<T>(this);
        m_discriminatorServerProxyModel = new ServerProxyModel<B>(this);
        m_discriminatorServerProxyModel->setSourceModel(m_discriminatorProxyModel);
        m_filterProxyModel = new FilterProxyModel(this);
        m_filterProxyModel->setDiscriminatorModel(m_discriminatorProxyModel);
        m_filterServerProxyModel = new ServerProxyModel<QIdentityProxyModel>(this);
        m_filterServerProxyModel->setSourceModel(m_filterProxyModel);
        connect(m_discriminatorProxyModel,
                &QAbstractItemModel::dataChanged,
                m_filterProxyModel,
                &QSortFilterProxyModel::invalidate);
    }

    void setDiscriminationRole(int role) { m_discriminatorProxyModel->setDiscriminationRole(role); }
    void setSourceModel(QAbstractItemModel *model)
    {
        m_discriminatorProxyModel->setSourceModel(model);
        m_filterProxyModel->setSourceModel(model);
    }

    void initialise(Probe *probe)
    {
        probe->registerModel(CI::filterModelId(name()), m_discriminatorServerProxyModel);
        probe->registerModel(CI::filteredModelId(name()), m_filterProxyModel);
    }
};
} // namespace GammaRay

#endif // GAMMARAY_CONNECTIVITYINSPECTOR_RECORDINGPROXYMODEL_H
