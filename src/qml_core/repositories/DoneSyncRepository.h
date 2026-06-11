#pragma once

#include <QSqlDatabase>

class DoneSyncRepository
{
public:
    explicit DoneSyncRepository(QSqlDatabase db);

    // Returns true on success. Outputs how many rows were updated.
    bool syncDoneFromTestRequests(int* compoundsUpdated,
                                 int* batchesUpdated,
                                 QString* errorOut);

private:
    QSqlDatabase db_;
};
