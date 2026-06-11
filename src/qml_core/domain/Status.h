#pragma once

#include <QString>

enum class Status : int {
    Pending = 0,
    Sent    = 1,
    Done    = 2
};

inline Status statusFromDbValue(const QString& v)
{
    const QString s = v.trimmed().toLower();
    if (s == "pending") return Status::Pending;
    if (s == "sent")    return Status::Sent;
    if (s == "done")    return Status::Done;
    return Status::Pending; // safe default
}

inline QString statusToDbValue(Status s)
{
    switch (s) {
    case Status::Pending: return "pending";
    case Status::Sent:    return "sent";
    case Status::Done:    return "done";
    }
    return "pending";
}

inline QString statusToText(Status s)
{
    switch (s) {
    case Status::Pending: return "Pending";
    case Status::Sent:    return "Sent";
    case Status::Done:    return "Done";
    }
    return "Pending";
}
