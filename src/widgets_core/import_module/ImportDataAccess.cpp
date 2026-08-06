#include "ImportDataAccess.h"
#include "utils/UserSessionHelper.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QRegularExpression>
#include <QDebug>
#include <QDate>
#include <QSettings>
#include <cmath>

namespace import_module {

ImportDataAccess::ImportDataAccess() {}

int ImportDataAccess::importData(const QString& tableName, const QList<QVariantMap>& mappedRows, QString* errOut) {
    if (tableName == "bottles") {
        return insertBottles(mappedRows, errOut) ? mappedRows.size() : -1;
    } else if (tableName == "solutions") {
        return insertSolutions(mappedRows, errOut) ? mappedRows.size() : -1;
    }
    
    if (errOut) *errOut = "Unsupported table target: " + tableName;
    return -1;
}

QString ImportDataAccess::generateNextBottleId() const {
    QSqlQuery q("SELECT invenesis_bottle_id FROM public.bottles");
    int maxNum = 0;
    QRegularExpression rx("^B(\\d+)$");
    while (q.next()) {
        QString id = q.value(0).toString().trimmed();
        QRegularExpressionMatch match = rx.match(id);
        if (match.hasMatch()) {
            int num = match.captured(1).toInt();
            if (num > maxNum) {
                maxNum = num;
            }
        }
    }
    return QString("B%1").arg(maxNum + 1);
}

QString ImportDataAccess::generateNextSolutionId() const {
    QSqlQuery q("SELECT invenesis_solution_id FROM public.solutions");
    int maxNum = 0;
    QRegularExpression rx("^([st])(\\d+)$");
    while (q.next()) {
        QString id = q.value(0).toString().trimmed();
        QRegularExpressionMatch match = rx.match(id);
        if (match.hasMatch()) {
            QString letter = match.captured(1);
            int num = match.captured(2).toInt();
            // Map t to +10000 conceptually to find the true absolute max
            int absoluteVal = num + (letter == "t" ? 10000 : 0);
            if (absoluteVal > maxNum) {
                maxNum = absoluteVal;
            }
        }
    }
    
    int nextAbsolute = maxNum + 1;
    if (nextAbsolute >= 10000) {
        return QString("t%1").arg(nextAbsolute - 10000, 4, 10, QChar('0'));
    } else {
        return QString("s%1").arg(nextAbsolute);
    }
}

bool ImportDataAccess::insertBottles(const QList<QVariantMap>& rows, QString* errOut) {
    QSqlQuery q;
    q.prepare("SELECT COALESCE(MAX(bottle_id), 0) FROM public.bottles");
    if (!q.exec() || !q.next()) {
        if (errOut) *errOut = "Failed to get max bottle_id: " + q.lastError().text();
        return false;
    }
    int nextPk = q.value(0).toInt() + 1;

    // Use a transaction for bulk insert
    QSqlDatabase db = QSqlDatabase::database();
    db.transaction();

    QString nextInvId = generateNextBottleId();
    int currentInvNum = nextInvId.mid(1).toInt();

    QSqlQuery ins;
    ins.prepare(R"(
        INSERT INTO public.bottles (
            bottle_id, invenesis_bottle_id, product_name, pubchem_cid, 
            chemical_formula, supplier, supplier_article_id, supplier_batch_id, 
            molecular_weight, received_amount, amount_unit, purity, storage, 
            receival_date, expiration_date, project_code, remarks
        ) VALUES (
            :pk, :inv_id, :prod, :cid, :formula, :supplier, :art_id, :batch_id,
            :mw, :amt, :unit, :purity, :storage, :rec_date, :exp_date, :project, :remarks
        )
    )");

    QString currentUser = SessionUtils::getCurrentUsername();

    for (const auto& row : rows) {
        QString invId = row.value("invenesis_bottle_id").toString().trimmed();
        if (invId.isEmpty()) {
            invId = QString("B%1").arg(currentInvNum++);
        } else {
            // Check if the user-provided ID matches the Bxxxx pattern and advance our counter to avoid future collisions
            QRegularExpressionMatch match = QRegularExpression("^B(\\d+)$").match(invId);
            if (match.hasMatch()) {
                int n = match.captured(1).toInt();
                if (n >= currentInvNum) currentInvNum = n + 1;
            }
        }

        ins.bindValue(":pk", nextPk++);
        ins.bindValue(":inv_id", invId);
        ins.bindValue(":prod", row.value("product_name").toString().trimmed());
        ins.bindValue(":cid", row.value("pubchem_cid"));
        ins.bindValue(":formula", row.value("chemical_formula"));
        ins.bindValue(":supplier", row.value("supplier"));
        ins.bindValue(":art_id", row.value("supplier_article_id"));
        ins.bindValue(":batch_id", row.value("supplier_batch_id"));
        
        QVariant mw = row.value("molecular_weight");
        ins.bindValue(":mw", mw.toString().isEmpty() ? QVariant(QVariant::Double) : mw);
        
        QVariant amt = row.value("received_amount");
        ins.bindValue(":amt", amt.toString().isEmpty() ? QVariant(QVariant::Double) : amt);
        
        ins.bindValue(":unit", row.value("amount_unit"));
        
        QVariant pur = row.value("purity");
        ins.bindValue(":purity", pur.toString().isEmpty() ? QVariant(QVariant::Double) : pur);
        
        ins.bindValue(":storage", row.value("storage"));
        
        QDate today = QDate::currentDate();
        QVariant rDate = row.value("receival_date");
        if (rDate.toString().trimmed().isEmpty() || !rDate.isValid()) {
            ins.bindValue(":rec_date", today);
        } else {
            ins.bindValue(":rec_date", rDate);
        }
        
        QVariant eDate = row.value("expiration_date");
        if (eDate.toString().trimmed().isEmpty() || !eDate.isValid()) {
            ins.bindValue(":exp_date", today.addYears(1));
        } else {
            ins.bindValue(":exp_date", eDate);
        }
        
        ins.bindValue(":project", row.value("project_code"));

        QVariant remarksVal = row.value("remarks");
        if (remarksVal.toString().trimmed().isEmpty()) {
            ins.bindValue(":remarks", currentUser);
        } else {
            ins.bindValue(":remarks", remarksVal);
        }

        if (!ins.exec()) {
            if (errOut) *errOut = "Failed to insert bottle: " + ins.lastError().text();
            db.rollback();
            return false;
        }
    }

    db.commit();
    return true;
}

bool ImportDataAccess::insertSolutions(const QList<QVariantMap>& rows, QString* errOut) {
    QSqlQuery q;
    q.prepare("SELECT COALESCE(MAX(solution_id), 0) FROM public.solutions");
    if (!q.exec() || !q.next()) {
        if (errOut) *errOut = "Failed to get max solution_id: " + q.lastError().text();
        return false;
    }
    int nextPk = q.value(0).toInt() + 1;

    QSqlDatabase db = QSqlDatabase::database();
    db.transaction();

    QString nextInvId = generateNextSolutionId();
    int currentAbsVal = 0;
    
    QRegularExpressionMatch match = QRegularExpression("^([st])(\\d+)$").match(nextInvId);
    if (match.hasMatch()) {
        currentAbsVal = match.captured(2).toInt() + (match.captured(1) == "t" ? 10000 : 0);
    }

    QSqlQuery ins;
    ins.prepare(R"(
        INSERT INTO public.solutions (
            solution_id, invenesis_solution_id, product_name, concentration, 
            concentration_unit, solvent, quantity, quantity_unit, container_id, 
            well_id, matrix_tube_id, preparation_date, expiration_date, project_code, prepared_by
        ) VALUES (
            :pk, :inv_id, :prod, :conc, :conc_unit, :solvent, :qty, :qty_unit,
            :container, :well, :tube, :prep_date, :exp_date, :project, :prep_by
        )
    )");

    QString currentUser = SessionUtils::getCurrentUsername();

    for (const auto& row : rows) {
        QString invId = row.value("invenesis_solution_id").toString().trimmed();
        if (invId.isEmpty()) {
            if (currentAbsVal >= 10000) {
                invId = QString("t%1").arg(currentAbsVal - 10000, 4, 10, QChar('0'));
            } else {
                invId = QString("s%1").arg(currentAbsVal);
            }
            currentAbsVal++;
        } else {
            // Update counter if user provides something larger
            QRegularExpressionMatch userMatch = QRegularExpression("^([st])(\\d+)$").match(invId);
            if (userMatch.hasMatch()) {
                int n = userMatch.captured(2).toInt() + (userMatch.captured(1) == "t" ? 10000 : 0);
                if (n >= currentAbsVal) currentAbsVal = n + 1;
            }
        }

        ins.bindValue(":pk", nextPk++);
        ins.bindValue(":inv_id", invId);
        ins.bindValue(":prod", row.value("product_name").toString().trimmed());
        
        QVariant conc = row.value("concentration");
        ins.bindValue(":conc", conc.toString().isEmpty() ? QVariant(QVariant::Double) : conc);
        
        ins.bindValue(":conc_unit", row.value("concentration_unit"));
        ins.bindValue(":solvent", row.value("solvent"));
        
        QVariant qty = row.value("quantity");
        ins.bindValue(":qty", qty.toString().isEmpty() ? QVariant(QVariant::Double) : qty);
        
        ins.bindValue(":qty_unit", row.value("quantity_unit"));
        ins.bindValue(":container", row.value("container_id"));
        ins.bindValue(":well", row.value("well_id"));
        ins.bindValue(":tube", row.value("matrix_tube_id"));
        
        QDate today = QDate::currentDate();
        QVariant pDate = row.value("preparation_date");
        if (pDate.toString().trimmed().isEmpty() || !pDate.isValid()) {
            ins.bindValue(":prep_date", today);
        } else {
            ins.bindValue(":prep_date", pDate);
        }

        QVariant eDate = row.value("expiration_date");
        if (eDate.toString().trimmed().isEmpty() || !eDate.isValid()) {
            ins.bindValue(":exp_date", today.addYears(1));
        } else {
            ins.bindValue(":exp_date", eDate);
        }

        ins.bindValue(":project", row.value("project_code"));

        QVariant prepBy = row.value("prepared_by");
        if (prepBy.toString().trimmed().isEmpty()) {
            ins.bindValue(":prep_by", currentUser);
        } else {
            ins.bindValue(":prep_by", prepBy);
        }

        if (!ins.exec()) {
            if (errOut) *errOut = "Failed to insert solution: " + ins.lastError().text();
            db.rollback();
            return false;
        }
    }

    db.commit();
    return true;
}

} // namespace import_module
