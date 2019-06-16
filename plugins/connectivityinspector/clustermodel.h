/*
  clustermodel.h

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

#ifndef CLUSTERMODEL_H
#define CLUSTERMODEL_H

#include <QAbstractItemModel>
#include <QVector>

namespace GammaRay {
namespace Connectivity {

struct ClusterItem
{
    quint64 id;
    const QString label;
};

class ClusterModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum Role { LabelRole = Qt::DisplayRole, VertexIdRole = Qt::UserRole + 1, ClusterIdRole };

    explicit ClusterModel(QObject *parent = nullptr);
    ~ClusterModel() override;

    void setClusterItems(const QVector<ClusterItem> &items);

    // Basic functionality:
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QMap<int, QVariant> itemData(const QModelIndex &index) const override;

private:
    QVector<ClusterItem> m_items;
};

} // namespace Connectivity
} // namespace GammaRay

#endif // CLUSTERMODEL_H
