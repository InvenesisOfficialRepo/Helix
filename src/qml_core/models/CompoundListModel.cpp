#include "CompoundListModel.h"
#include "domain/Status.h"

CompoundListModel::CompoundListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int CompoundListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return items_.size();
}

QVariant CompoundListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= items_.size())
        return {};

    const auto &it = items_.at(index.row());
    switch (role) {
    case TrackedCompoundIdRole: return it.trackedCompoundId;
    case CompoundNameRole:      return it.compoundName;
    case StatusRole:            return static_cast<int>(it.status);
    case StatusTextRole:        return statusToText(it.status);
    case SelectedRole:          return it.selected;
    case SelectableRole:        return (it.status == Status::Pending && it.hasSolution);
    default:                    return {};
    }
}

QHash<int, QByteArray> CompoundListModel::roleNames() const
{
    return {
        {TrackedCompoundIdRole, "trackedCompoundId"},
        {CompoundNameRole, "compoundName"},
        {StatusRole, "status"},
        {StatusTextRole, "statusText"},
        {SelectedRole, "selected"},
        {SelectableRole, "selectable"}
    };
}

void CompoundListModel::setItems(QVector<CompoundItem> items)
{
    beginResetModel();
    items_ = std::move(items);
    endResetModel();
}

void CompoundListModel::clearSelection()
{
    for (int i = 0; i < items_.size(); ++i) {
        if (items_[i].selected) {
            items_[i].selected = false;
            const auto idx = index(i);
            emit dataChanged(idx, idx, {SelectedRole});
        }
    }
}

void CompoundListModel::setSelected(int row, bool selected)
{
    if (row < 0 || row >= items_.size()) return;
    if (items_[row].status != Status::Pending || !items_[row].hasSolution) return;

    if (items_[row].selected == selected) return;
    items_[row].selected = selected;

    const auto idx = index(row);
    emit dataChanged(idx, idx, {SelectedRole});
}

void CompoundListModel::selectAllPending()
{
    for (int i = 0; i < items_.size(); ++i) {
        if (items_[i].status == Status::Pending && items_[i].hasSolution && !items_[i].selected) {
            items_[i].selected = true;
            const auto idx = index(i);
            emit dataChanged(idx, idx, {SelectedRole});
        }
    }
}

QStringList CompoundListModel::selectedIds() const
{
    QStringList out;
    for (const auto &it : items_) {
        if (it.selected && it.status == Status::Pending)
            out.push_back(it.trackedCompoundId);
    }
    return out;
}
