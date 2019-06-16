#include "clustermodel.h"

#include "connectivityinspectorcommon.h"

using namespace GammaRay::Connectivity;

ClusterModel::ClusterModel(QObject *parent)
    : QAbstractItemModel(parent)
{}

ClusterModel::~ClusterModel() = default;

QModelIndex ClusterModel::index(int row, int column, const QModelIndex &parent) const
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

QModelIndex ClusterModel::parent(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return {};
}

int ClusterModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_items.count();
}

int ClusterModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return 1;
}

QVariant ClusterModel::data(const QModelIndex &index, int role) const
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
    if (role == ClusterIdRole)
        return QVariant::fromValue<quint64>(m_items.at(index.row()).id);
    return {};
}

QMap<int, QVariant> ClusterModel::itemData(const QModelIndex &index) const
{
    if (!index.isValid())
        return {};
    QMap<int, QVariant> map = QAbstractItemModel::itemData(index);
    map.insert(ClusterIdRole, this->data(index, ClusterIdRole));
    return map;
}

void ClusterModel::setClusterItems(const QVector<ClusterItem> &items)
{
    beginResetModel();
    m_items = items;
    endResetModel();
}
