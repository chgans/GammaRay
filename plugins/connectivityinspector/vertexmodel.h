#ifndef VERTEXMODEL_H
#define VERTEXMODEL_H

#include <QAbstractItemModel>
#include <QVector>

namespace GammaRay {
namespace Connectivity {

struct VertexItem
{
    quint64 id;
    QString label;
    quint64 clusterId;
};

class VertexModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit VertexModel(QObject *parent = nullptr);
    ~VertexModel() override;

    void setVertexItems(const QVector<VertexItem> &items);

    // Basic functionality:
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QMap<int, QVariant> itemData(const QModelIndex &index) const override;

private:
    QVector<VertexItem> m_items;
};

} // namespace Connectivity
} // namespace GammaRay

#endif // VERTEXMODEL_H
