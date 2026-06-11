#pragma once

#include <QAbstractListModel>
#include <QDate>
#include <QVector>
#include "CalendarEventsModel.h"
#include "models/CalendarEventsModel.h"

class TwoWeekDayModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        DayRole = Qt::UserRole + 1,
        IsoRole,
        TotalCountRole,
        EventsModelRole
    };

    explicit TwoWeekDayModel(QObject* parent = nullptr)
        : QAbstractListModel(parent)
    {
        // Create 14 persistent child models (no leaks, no churn on refresh)
        rows_.reserve(14);
        for (int i = 0; i < 14; ++i) {
            DayRow r;
            r.eventsModel = new CalendarEventsModel(this);
            rows_.push_back(r);
        }
    }

    int rowCount(const QModelIndex& parent = {}) const override {
        if (parent.isValid()) return 0;
        return rows_.size(); // always 14
    }

    QVariant data(const QModelIndex& index, int role) const override {
        if (!index.isValid() || index.row() < 0 || index.row() >= rows_.size())
            return {};

        const auto& r = rows_.at(index.row());
        switch (role) {
        case DayRole:        return r.day;
        case IsoRole:        return r.day.toString(Qt::ISODate);
        case TotalCountRole: return r.totalCount;
        case EventsModelRole:
            // Return as QObject* so QVariant/QML is happy; QML will still treat it as a model.
            return QVariant::fromValue(static_cast<QObject*>(r.eventsModel));
        default:
            return {};
        }
    }

    QHash<int, QByteArray> roleNames() const override {
        return {
            {DayRole, "day"},
            {IsoRole, "iso"},
            {TotalCountRole, "totalCount"},
            {EventsModelRole, "eventsModel"},
        };
    }

    // Called by your WallboardViewModel after it loads all events for the 14-day range
    void setTwoWeekData(const QDate& startMonday, const QVector<CalendarEventItem>& allEvents)
    {
        // bucket by day
        QMap<QDate, QVector<CalendarEventItem>> bucket;
        for (const auto& it : allEvents)
            bucket[it.scheduledFor].push_back(it);

        beginResetModel();
        for (int i = 0; i < 14; ++i) {
            auto& r = rows_[i];
            r.day = startMonday.addDays(i);

            auto items = bucket.value(r.day);
            r.totalCount = items.size();
            r.eventsModel->setItems(std::move(items));
        }
        endResetModel();
    }

private:
    struct DayRow {
        QDate day;
        int totalCount = 0;
        CalendarEventsModel* eventsModel = nullptr;
    };

    QVector<DayRow> rows_;
};
