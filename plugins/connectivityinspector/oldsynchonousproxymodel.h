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

    bool isSynchronised() const;

public slots:
    void synchronise();

signals:
    void synchronisationStarted();
    void synchronisationFinished();

    // QAbstractProxyModel interface
public:
    void setSourceModel(QAbstractItemModel *sourceModel) override;
    QModelIndex mapToSource(const QModelIndex &proxyIndex) const override;
    QModelIndex mapFromSource(const QModelIndex &sourceIndex) const override;

    // QAbstractItemModel interface
public:
    int rowCount(const QModelIndex &parent) const override;

private:
    QHash<int, QSet<int>> m_requirements;
    QSet<int> m_pendingSourceRows;
    bool m_isSynchronised = false;
    void clear();

private slots:
    void addRows(const QModelIndex &index, int first, int last);
    void removeRows(const QModelIndex &index, int first, int last);
    void updateRows(const QModelIndex &topLeft,
                    const QModelIndex &bottomRight,
                    const QVector<int> &roles);

    // QSortFilterProxyModel interface
protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
};

} // namespace GammaRay
#endif // SYNCHONOUSPROXYMODEL_H
