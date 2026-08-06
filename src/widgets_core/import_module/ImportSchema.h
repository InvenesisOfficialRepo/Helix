#pragma once

#include <QString>
#include <QStringList>
#include <QList>
#include <QVariant>

namespace import_module {

enum class ImportDataType {
    String,
    Number,
    Date
};

struct ImportColumn {
    QString dbColumnName;
    QString displayName;
    bool isRequired;
    ImportDataType dataType;
    QStringList aliases;
};

struct ImportTableSchema {
    QString tableName;
    QString primaryIdPrefix; // "B" for bottles, "s"/"t" for solutions
    QString primaryIdColumn; // e.g. "invenesis_bottle_id"
    QList<ImportColumn> columns;
    
    // Helper
    const ImportColumn* getColumn(const QString& dbCol) const {
        for (const auto& col : columns) {
            if (col.dbColumnName == dbCol) return &col;
        }
        return nullptr;
    }
};

class SchemaFactory {
public:
    static ImportTableSchema createBottlesSchema() {
        ImportTableSchema schema;
        schema.tableName = "bottles";
        schema.primaryIdPrefix = "B";
        schema.primaryIdColumn = "invenesis_bottle_id";
        
        schema.columns = {
            {"invenesis_bottle_id", "Bottle ID", false, ImportDataType::String, 
             {"bottle id", "bottle", "invenesis id", "id"}},
             
            {"product_name", "Product Name", true, ImportDataType::String, 
             {"name", "product name", "product", "compound name", "compound", "material", "substance"}},
             
            {"pubchem_cid", "PubChem CID", false, ImportDataType::String, 
             {"pubchem cid", "pubchem", "cid"}},
             
            {"chemical_formula", "Chemical Formula", false, ImportDataType::String, 
             {"formula", "chemical formula"}},
             
            {"supplier", "Supplier", false, ImportDataType::String, 
             {"supplier", "vendor", "manufacturer", "provider"}},
             
            {"supplier_article_id", "Supplier Article ID", false, ImportDataType::String, 
             {"supplier article id", "article no", "article nb", "catalog no", "cat number", "sku", "provider id"}},
             
            {"supplier_batch_id", "Supplier Batch ID", false, ImportDataType::String, 
             {"supplier batch id", "batch no", "batch nb", "lot number", "lot no", "batch"}},
             
            {"molecular_weight", "Molecular Weight", false, ImportDataType::Number, 
             {"mw", "molecular weight", "weight"}},
             
            {"received_amount", "Received Amount", false, ImportDataType::Number, 
             {"amount", "qty", "quantity", "received amount", "mass"}},
             
            {"amount_unit", "Amount Unit", false, ImportDataType::String, 
             {"unit", "amount unit", "quantity unit"}},
             
            {"purity", "Purity (%)", false, ImportDataType::Number, 
             {"purity", "purity (%)", "purity %"}},
             
            {"storage", "Storage", false, ImportDataType::String, 
             {"storage", "storage condition", "storage temp", "temp"}},
             
            {"receival_date", "Receival Date", false, ImportDataType::Date, 
             {"receival date", "received date", "date received", "arrival date"}},
             
            {"expiration_date", "Expiration Date", false, ImportDataType::Date, 
             {"expiration date", "expiry date", "exp date", "valid until"}},
             
            {"project_code", "Project Code", false, ImportDataType::String, 
             {"project code", "project", "code"}},
             
            {"remarks", "Remarks", false, ImportDataType::String, 
             {"remarks", "remark", "notes", "user", "comment", "comments"}}
        };
        return schema;
    }

    static ImportTableSchema createSolutionsSchema() {
        ImportTableSchema schema;
        schema.tableName = "solutions";
        schema.primaryIdPrefix = "s"; // s or t depends on ID > 9999
        schema.primaryIdColumn = "invenesis_solution_id";
        
        schema.columns = {
            {"invenesis_solution_id", "Solution ID", false, ImportDataType::String, 
             {"solution id", "solution", "invenesis id", "id"}},
             
            {"product_name", "Product Name", true, ImportDataType::String, 
             {"name", "product name", "product", "compound name", "compound", "material", "substance"}},
             
            {"concentration", "Concentration", false, ImportDataType::Number, 
             {"concentration", "conc", "stock conc"}},
             
            {"concentration_unit", "Concentration Unit", false, ImportDataType::String, 
             {"conc unit", "concentration unit", "unit"}},
             
            {"solvent", "Solvent", false, ImportDataType::String, 
             {"solvent", "diluent"}},
             
            {"quantity", "Volume / Quantity", false, ImportDataType::Number, 
             {"volume", "vol", "qty", "quantity"}},
             
            {"quantity_unit", "Volume Unit", false, ImportDataType::String, 
             {"vol unit", "volume unit", "quantity unit", "qty unit", "quantity_unit"}},
             
            {"container_id", "Container ID (Plate)", false, ImportDataType::String, 
             {"plate", "plate id", "rack id", "container id", "rack barcode"}},
             
            {"well_id", "Well ID", false, ImportDataType::String, 
             {"well", "well id", "well position", "position"}},
             
            {"matrix_tube_id", "Tube Barcode", false, ImportDataType::String, 
             {"barcode", "tube barcode", "matrix tube id", "tube id"}},
             
            {"preparation_date", "Preparation Date", false, ImportDataType::Date, 
             {"prep date", "preparation date", "date prepped"}},
             
            {"expiration_date", "Expiration Date", false, ImportDataType::Date, 
             {"expiration date", "expiry date", "exp date", "valid until"}},
             
            {"project_code", "Project Code", false, ImportDataType::String, 
             {"project code", "project", "code"}},
             
            {"prepared_by", "Prepared By", false, ImportDataType::String, 
             {"prepared by", "user", "owner", "created by"}}
        };
        return schema;
    }
};

} // namespace import_module
