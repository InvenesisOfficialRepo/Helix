#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QVector>

class ProjectModel;

struct TestCardItem {
    QString testCode;
    int pendingCount = 0;
    int sentCount = 0;
    ProjectModel* projectsModel = nullptr; // owns batch rows
};

class TestCardModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        TestCodeRole = Qt::UserRole + 1,
        PendingCountRole,
        SentCountRole,
        ProjectsModelRole
    };

    explicit TestCardModel(QObject* parent = nullptr);
    ~TestCardModel() override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setItems(QVector<TestCardItem> items);

private:
    void clearOwnedModels();

    QVector<TestCardItem> items_;
};
