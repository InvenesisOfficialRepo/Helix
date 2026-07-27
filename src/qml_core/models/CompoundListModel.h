#pragma once

#include <QAbstractListModel>
#include <QStringList>
#include <QVector>

#include "domain/Status.h"

struct CompoundItem {
    QString trackedCompoundId;  // uuid string
    QString compoundName;
    QString species;
    QString solvent;
    Status status = Status::Pending;
    bool selected = false;
    bool hasSolution = false;
    int numberOfDilutions = 3;
    int numberOfReplicates = 3;
};

class CompoundListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QStringList availableSpecies READ availableSpecies NOTIFY compoundsChanged)
    Q_PROPERTY(QString speciesFilter READ speciesFilter WRITE setSpeciesFilter NOTIFY speciesFilterChanged)
    Q_PROPERTY(QStringList availableSolvents READ availableSolvents NOTIFY compoundsChanged)

public:
    enum Roles {
        TrackedCompoundIdRole = Qt::UserRole + 1,
        CompoundNameRole,
        SpeciesRole,
        SolventRole,
        StatusRole,
        StatusTextRole,
        SelectedRole,
        SelectableRole,
        NumberOfDilutionsRole,
        NumberOfReplicatesRole
    };

    explicit CompoundListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void setSelected(int row, bool selected);
    Q_INVOKABLE void selectAllPending();
    Q_INVOKABLE void selectAllBySpecies(const QString& species);
    Q_INVOKABLE void selectAllBySolvent(const QString& solvent);
    Q_INVOKABLE QStringList selectedIds() const;
    QStringList availableSpecies() const;
    QStringList availableSolvents() const;

    QString speciesFilter() const;
    Q_INVOKABLE void setSpeciesFilter(const QString& species);

    void setItems(QVector<CompoundItem> items);
    void clearSelection();

signals:
    void compoundsChanged();
    void speciesFilterChanged();

private:
    void updateVisibleIndices();
    
    QVector<CompoundItem> items_;
    QString speciesFilter_;
    QVector<int> visibleIndices_;
};
