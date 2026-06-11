#pragma once

#include <QAbstractListModel>
#include <QStringList>
#include <QVector>

#include "domain/Status.h"

struct CompoundItem {
    QString trackedCompoundId;  // uuid string
    QString compoundName;
    Status status = Status::Pending;
    bool selected = false;
    bool hasSolution = false;
};

class CompoundListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        TrackedCompoundIdRole = Qt::UserRole + 1,
        CompoundNameRole,
        StatusRole,
        StatusTextRole,
        SelectedRole,
        SelectableRole
    };

    explicit CompoundListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void setSelected(int row, bool selected);
    Q_INVOKABLE void selectAllPending();
    Q_INVOKABLE QStringList selectedIds() const;

    void setItems(QVector<CompoundItem> items);
    void clearSelection();

private:
    QVector<CompoundItem> items_;
};
