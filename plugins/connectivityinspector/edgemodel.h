
#ifndef EDGEMODEL_H
#define EDGEMODEL_H

#include <QAbstractItemModel>

#include <QAbstractItemModel>
#include <QVector>

namespace GammaRay {
namespace Connectivity {

struct EdgeItem
{
    quint64 id;
    QString label;
    quint64 sourceId;
    quint64 targetId;
    int weight;
};

class EdgeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit EdgeModel(QObject *parent = nullptr);
    ~EdgeModel() override;

    void setEdgeItems(const QVector<EdgeItem> &items);

    // Basic functionality:
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QMap<int, QVariant> itemData(const QModelIndex &index) const override;

private:
    QVector<EdgeItem> m_items;
};

} // namespace Connectivity
} // namespace GammaRay

#endif // EDGEMODEL_H
