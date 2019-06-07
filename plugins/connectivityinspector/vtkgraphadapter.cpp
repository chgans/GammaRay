/*
  vtkgraphadapter.cpp

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

#include "vtkgraphadapter.h"

#include "connectivityinspectorcommon.h"

#include "connectionmodel.h"

#include "common/objectid.h"

#include <vtkIntArray.h>
#include <vtkMutableDirectedGraph.h>
#include <vtkStringArray.h>
#include <vtkUnsignedLongLongArray.h>

#include <QAbstractItemModel>

using namespace GammaRay;

namespace {
const char *s_ObjectIdArrayName = "ObjectId";
const char *s_ThreadIdArrayName = "ThreadId";
const char *s_ObjectLabelArrayName = "ObjectLabel";
const char *s_ConnWeightArrayName = "ConnWeight";

inline quint64 objectId(QAbstractItemModel *model, int row, int col)
{
    constexpr int role = ConnectionModel::ObjectIdRole;
    const QModelIndex index = model->index(row, col);
    return index.data(role).value<ObjectId>().id();
}

inline quint64 threadId(QAbstractItemModel *model, int row, int col)
{
    constexpr int role = ConnectionModel::ThreadIdRole;
    const QModelIndex index = model->index(row, col);
    return index.data(role).value<ObjectId>().id();
}

inline QString objectLabel(QAbstractItemModel *model, int row, int col)
{
    constexpr int role = Qt::DisplayRole;
    const QModelIndex index = model->index(row, col);
    return index.data(role).toString();
}

inline int weight(QAbstractItemModel *model, int row)
{
    constexpr int role = Qt::DisplayRole;
    constexpr int col = ConnectionModel::CountColumn;
    const QModelIndex index = model->index(row, col);
    return index.data(role).toInt();
}

} // namespace

vtkGraphAdapter::vtkGraphAdapter(QObject *parent)
    : QObject(parent)
{
    m_objectIdArray = vtkUnsignedLongLongArray::New();
    m_objectIdArray->SetName(s_ObjectIdArrayName);
    m_threadIdArray = vtkUnsignedLongLongArray::New();
    m_threadIdArray->SetName(s_ThreadIdArrayName);
    m_objectLabelArray = vtkStringArray::New();
    m_objectLabelArray->SetName(s_ObjectLabelArrayName);
    m_connWeightArray = vtkIntArray::New();
    m_connWeightArray->SetName(s_ConnWeightArrayName);
}

vtkGraphAdapter::~vtkGraphAdapter() {}

void GammaRay::vtkGraphAdapter::setSourceModel(const QAbstractItemModel *model)
{
    m_input = model;
}

vtkGraph *vtkGraphAdapter::graph()
{
    return m_output;
}

void vtkGraphAdapter::update() {}
