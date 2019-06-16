/*
  vertexmodel.cpp

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

#include "vertexmodel.h"

#include "connectivityinspectorcommon.h"

#include <QDebug>

using namespace GammaRay::Connectivity;

VertexModel::VertexModel(QObject *parent)
    : QAbstractItemModel(parent)
{
}

VertexModel::~VertexModel() = default;

QModelIndex VertexModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid())
        return {};
    if (row < 0)
        return {};
    if (row >= m_items.count())
        return {};
    if (column != 0)
        return {};
    return createIndex(row, column);
}

QModelIndex VertexModel::parent(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return {};
}

int VertexModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_items.count();
}

int VertexModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return 1;
}

QVariant VertexModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};
    if (index.column() != 0)
        return {};
    if (index.row() < 0)
        return {};
    if (index.row() >= m_items.count())
        return {};
    if (role == LabelRole)
        return m_items.at(index.row()).label;
    if (role == VertexIdRole) {
        qDebug() << index.row() << QVariant(m_items.at(index.row()).id);
        return m_items.at(index.row()).id;
    }
    if (role == ClusterIdRole)
        return m_items.at(index.row()).clusterId;
    return {};
}

QMap<int, QVariant> VertexModel::itemData(const QModelIndex &index) const
{
    if (!index.isValid())
        return {};
    QMap<int, QVariant> map = QAbstractItemModel::itemData(index);
    map.insert(VertexIdRole, this->data(index, VertexIdRole));
    map.insert(ClusterIdRole, this->data(index, ClusterIdRole));
    return map;
}

void VertexModel::setVertexItems(const QVector<VertexItem> &items)
{
    beginResetModel();
    m_items = items;
    endResetModel();
}
