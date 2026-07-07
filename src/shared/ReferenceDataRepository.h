#ifndef REFERENCEDATAREPOSITORY_H
#define REFERENCEDATAREPOSITORY_H

#include <QSqlDatabase>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>

class ReferenceDataRepository {
public:
    explicit ReferenceDataRepository(const QSqlDatabase& db);

    QJsonObject getCatalogue() const;
    QJsonObject getQcPlates() const;
    QJsonArray getStandardsMatrix() const;
    QJsonObject getTestsVocabulary() const;

private:
    QSqlDatabase db_;
};

#endif // REFERENCEDATAREPOSITORY_H
