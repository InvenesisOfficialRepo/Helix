import json
import os

data_dir = r"C:\Users\Maxime\Documents\InvenesisMergeApp\resources\data"

def escape_sql(val):
    if val is None:
        return "NULL"
    if isinstance(val, str):
        return "'" + val.replace("'", "''") + "'"
    if isinstance(val, bool):
        return "TRUE" if val else "FALSE"
    return str(val)

sql = []

sql.append("-- 1. tests_vocabulary")
sql.append("""CREATE TABLE IF NOT EXISTS tests_vocabulary (
    test_id VARCHAR(50) PRIMARY KEY,
    plate_size INTEGER
);""")
with open(os.path.join(data_dir, "tests_vocabulary.json"), "r", encoding="utf-8") as f:
    vocab = json.load(f)
    for test_id, plate_size in vocab.items():
        sql.append(f"INSERT INTO tests_vocabulary (test_id, plate_size) VALUES ({escape_sql(test_id)}, {escape_sql(int(plate_size))}) ON CONFLICT DO NOTHING;")

sql.append("\n-- 2. test_catalogue & test_standards")
sql.append("""CREATE TABLE IF NOT EXISTS test_catalogue (
    test_id VARCHAR(50) PRIMARY KEY REFERENCES tests_vocabulary(test_id),
    concentration_unit VARCHAR(50),
    is_tarsal BOOLEAN,
    vol_from_daughter_ul FLOAT,
    total_well_vol_ul FLOAT,
    well_area_m2 FLOAT
);""")

sql.append("""CREATE TABLE IF NOT EXISTS test_standards (
    test_id VARCHAR(50) REFERENCES test_catalogue(test_id),
    standard_name VARCHAR(100),
    top_dose_um FLOAT,
    ec50_um FLOAT,
    source VARCHAR(100),
    PRIMARY KEY (test_id, standard_name)
);""")

with open(os.path.join(data_dir, "invenesis_catalogue.json"), "r", encoding="utf-8") as f:
    cat = json.load(f)
    for test_id, data in cat.items():
        if test_id not in vocab:
             # some test ids might be in catalogue but not in vocab, let's insert them into vocab just in case
             sql.append(f"INSERT INTO tests_vocabulary (test_id, plate_size) VALUES ({escape_sql(test_id)}, NULL) ON CONFLICT DO NOTHING;")
             
        conc_unit = data.get("concentration_unit")
        is_tarsal = data.get("is_tarsal")
        vol_from = data.get("vol_from_daughter_µl")
        total_well = data.get("total_well_vol_µl")
        well_area = data.get("well_area_m2")
        
        sql.append(f"INSERT INTO test_catalogue (test_id, concentration_unit, is_tarsal, vol_from_daughter_ul, total_well_vol_ul, well_area_m2) VALUES ({escape_sql(test_id)}, {escape_sql(conc_unit)}, {escape_sql(is_tarsal)}, {escape_sql(vol_from)}, {escape_sql(total_well)}, {escape_sql(well_area)}) ON CONFLICT DO NOTHING;")
        
        standards = data.get("standards", {})
        for std_name, std_data in standards.items():
            top_dose = std_data.get("top_dose_uM")
            ec50 = std_data.get("ec50_uM")
            source = std_data.get("source")
            sql.append(f"INSERT INTO test_standards (test_id, standard_name, top_dose_um, ec50_um, source) VALUES ({escape_sql(test_id)}, {escape_sql(std_name)}, {escape_sql(top_dose)}, {escape_sql(ec50)}, {escape_sql(source)}) ON CONFLICT DO NOTHING;")

sql.append("\n-- 3. qc_plates")
sql.append("""CREATE TABLE IF NOT EXISTS qc_plates (
    id SERIAL PRIMARY KEY,
    name VARCHAR(100),
    type VARCHAR(10),
    standard_name VARCHAR(100),
    concentration FLOAT
);""")
with open(os.path.join(data_dir, "qc_plates.json"), "r", encoding="utf-8") as f:
    qc = json.load(f)
    for name, data in qc.items():
        for type_, details in data.items():
            std_name = details.get("standard")
            conc = details.get("conc")
            sql.append(f"INSERT INTO qc_plates (name, type, standard_name, concentration) VALUES ({escape_sql(name)}, {escape_sql(type_)}, {escape_sql(std_name)}, {escape_sql(conc)});")

sql.append("\n-- 4. standards_matrix")
sql.append("""CREATE TABLE IF NOT EXISTS standards_matrix (
    id SERIAL PRIMARY KEY,
    container_barcode VARCHAR(100),
    sample_alias VARCHAR(100),
    container_position VARCHAR(50),
    volume FLOAT,
    volume_unit VARCHAR(50),
    concentration FLOAT,
    concentration_unit VARCHAR(50),
    userdef_value_1 VARCHAR(100),
    invenesis_solution_id VARCHAR(100)
);""")
with open(os.path.join(data_dir, "standards_matrix.json"), "r", encoding="utf-8") as f:
    matrix = json.load(f)
    for item in matrix:
        barcode = item.get("Containerbarcode")
        alias = item.get("Samplealias")
        pos = item.get("Containerposition")
        vol = item.get("Volume")
        vol_unit = item.get("VolumeUnit")
        conc = item.get("Concentration")
        if conc is not None:
             try:
                 conc = float(conc)
             except:
                 conc = None
        conc_unit = item.get("ConcentrationUnit")
        userdef = item.get("UserdefValue1")
        sol_id = item.get("invenesis_solution_ID")
        
        sql.append(f"INSERT INTO standards_matrix (container_barcode, sample_alias, container_position, volume, volume_unit, concentration, concentration_unit, userdef_value_1, invenesis_solution_id) VALUES ({escape_sql(barcode)}, {escape_sql(alias)}, {escape_sql(pos)}, {escape_sql(vol)}, {escape_sql(vol_unit)}, {escape_sql(conc)}, {escape_sql(conc_unit)}, {escape_sql(userdef)}, {escape_sql(sol_id)});")

out_path = r"C:\Users\Maxime\Documents\InvenesisDatabase info\migration_reference_data.sql"
with open(out_path, "w", encoding="utf-8") as f:
    f.write("\n".join(sql))

print(f"Generated {out_path}")
