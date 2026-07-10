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
    case SpeciesRole:           return it.species;
    case SolventRole:           return it.solvent;
    case StatusRole:            return static_cast<int>(it.status);
    case StatusTextRole:        return statusToText(it.status);
    case SelectedRole:          return it.selected;
    case SelectableRole:        return (it.status == Status::Pending && it.hasSolution);
    case NumberOfDilutionsRole: return it.numberOfDilutions;
    case NumberOfReplicatesRole:return it.numberOfReplicates;
    default:                    return {};
    }
}

QHash<int, QByteArray> CompoundListModel::roleNames() const
{
    return {
        {TrackedCompoundIdRole, "trackedCompoundId"},
        {CompoundNameRole, "compoundName"},
        {SpeciesRole, "species"},
        {SolventRole, "solvent"},
        {StatusRole, "status"},
        {StatusTextRole, "statusText"},
        {SelectedRole, "selected"},
        {SelectableRole, "selectable"},
        {NumberOfDilutionsRole, "numberOfDilutions"},
        {NumberOfReplicatesRole, "numberOfReplicates"}
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

void CompoundListModel::selectAllBySpecies(const QString& species)
{
    for (int i = 0; i < items_.size(); ++i) {
        if (items_[i].status == Status::Pending && items_[i].species == species && items_[i].hasSolution) {
            items_[i].selected = true;
        }
    }
    emit dataChanged(index(0, 0), index(items_.size() - 1, 0), {SelectedRole});
}

void CompoundListModel::selectAllBySolvent(const QString& solvent)
{
    for (int i = 0; i < items_.size(); ++i) {
        if (items_[i].status == Status::Pending && items_[i].solvent == solvent && items_[i].hasSolution) {
            items_[i].selected = true;
        }
    }
    emit dataChanged(index(0, 0), index(items_.size() - 1, 0), {SelectedRole});
}

QStringList CompoundListModel::availableSpecies() const
{
    QStringList out;
    for (const auto &it : items_) {
        if (!it.species.isEmpty() && !out.contains(it.species)) {
            out.push_back(it.species);
        }
    }
    out.sort();
    return out;
}

QStringList CompoundListModel::availableSolvents() const
{
    QStringList out;
    for (const auto &it : items_) {
        if (!it.solvent.isEmpty() && !out.contains(it.solvent)) {
            out.push_back(it.solvent);
        }
    }
    out.sort();
    return out;
}
