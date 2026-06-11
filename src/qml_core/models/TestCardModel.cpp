#include "TestCardModel.h"
#include "ProjectModel.h"

#include <QVariant>

TestCardModel::TestCardModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

TestCardModel::~TestCardModel()
{
    clearOwnedModels();
}

void TestCardModel::clearOwnedModels()
{
    for (auto &it : items_) {
        if (it.projectsModel) {
            it.projectsModel->deleteLater();
            it.projectsModel = nullptr;
        }
    }
}

int TestCardModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return items_.size();
}

QVariant TestCardModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= items_.size())
        return {};

    const auto &it = items_.at(index.row());
    switch (role) {
    case TestCodeRole:       return it.testCode;
    case PendingCountRole:   return it.pendingCount;
    case SentCountRole:      return it.sentCount;
    case ProjectsModelRole:  return QVariant::fromValue(static_cast<QObject*>(it.projectsModel));
    default:                 return {};
    }
}

QHash<int, QByteArray> TestCardModel::roleNames() const
{
    return {
        {TestCodeRole, "testCode"},
        {PendingCountRole, "pendingCount"},
        {SentCountRole, "sentCount"},
        {ProjectsModelRole, "projectsModel"}
    };
}

void TestCardModel::setItems(QVector<TestCardItem> items)
{
    beginResetModel();
    clearOwnedModels();
    items_ = std::move(items);
    endResetModel();
}
