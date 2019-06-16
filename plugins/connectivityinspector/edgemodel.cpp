#include "edgemodel.h"

#include "connectivityinspectorcommon.h"

using namespace GammaRay::Connectivity;

EdgeModel::EdgeModel(QObject *parent)
    : QAbstractItemModel(parent)
{}

EdgeModel::~EdgeModel() = default;

QModelIndex EdgeModel::index(int row, int column, const QModelIndex &parent) const
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

QModelIndex EdgeModel::parent(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return {};
}

int EdgeModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_items.count();
}

int EdgeModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return 1;
}

QVariant EdgeModel::data(const QModelIndex &index, int role) const
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
    if (role == EdgeIdRole)
        return m_items.at(index.row()).id;
    if (role == SourceIdRole)
        return m_items.at(index.row()).sourceId;
    if (role == TargetIdRole)
        return m_items.at(index.row()).targetId;
    if (role == WeightRole)
        return m_items.at(index.row()).weight;
    return {};
}

QMap<int, QVariant> EdgeModel::itemData(const QModelIndex &index) const
{
    if (!index.isValid())
        return {};
    QMap<int, QVariant> map = QAbstractItemModel::itemData(index);
    map.insert(EdgeIdRole, this->data(index, EdgeIdRole));
    map.insert(SourceIdRole, this->data(index, SourceIdRole));
    map.insert(TargetIdRole, this->data(index, TargetIdRole));
    map.insert(WeightRole, this->data(index, WeightRole));
    return map;
}

void EdgeModel::setEdgeItems(const QVector<EdgeItem> &items)
{
    beginResetModel();
    m_items = items;
    endResetModel();
}
