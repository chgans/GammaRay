/*
  synchonousproxymodel.h

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

#ifndef SYNCHONOUSPROXYMODEL_H
#define SYNCHONOUSPROXYMODEL_H

#include <QHash>
#include <QQueue>
#include <QSet>
#include <QSortFilterProxyModel>

namespace GammaRay {

class SynchonousProxyModel : public QSortFilterProxyModel
{
public:
    SynchonousProxyModel(QObject *parent = nullptr);
    ~SynchonousProxyModel() override;

    void addSynchronousRequirement(int column, int role);

    // QAbstractProxyModel interface
public:
    void setSourceModel(QAbstractItemModel *sourceModel) override;

    // QSortFilterProxyModel interface
protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    QHash<int, QSet<int>> m_requirements;
    void fetchRequiredData(const QModelIndex &parent, int first, int last) const;
    void fetchAllRequiredData(const QModelIndex &parent) const;
};

} // namespace GammaRay
#endif // SYNCHONOUSPROXYMODEL_H
