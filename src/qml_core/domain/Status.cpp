#include "Status.h"

QString statusToString(Status s)
{
    switch (s) {
    case Status::Pending: return "Pending";
    case Status::Sent:    return "Sent";
    case Status::Done:    return "Done";
    }
    return "Pending";
}

Status statusFromString(const QString &v)
{
    const auto t = v.trimmed().toLower();
    if (t == "sent") return Status::Sent;
    if (t == "done") return Status::Done;
    return Status::Pending;
}
