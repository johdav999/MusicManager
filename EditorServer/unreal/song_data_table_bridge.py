import json
import os
import sys
import traceback

import unreal


def get_arg(name, default=""):
    prefix = name + "="
    for arg in sys.argv:
        if arg.startswith(prefix):
            return arg[len(prefix):].strip('"')
    return default


def fail(code, message):
    unreal.log_error("MusicManager SongData bridge failed: {0}".format(message))
    print(json.dumps({"ok": False, "code": code, "message": message}))
    sys.exit(1)


def require_function(owner, name):
    if not hasattr(owner, name):
        fail(
            "UNREAL_API_MISSING",
            "Required Unreal Python function {0}.{1} is unavailable. Enable the Python Editor Script Plugin and DataTable editor scripting support.".format(
                owner.__name__ if hasattr(owner, "__name__") else str(owner),
                name,
            ),
        )
    return getattr(owner, name)


def load_data_table(asset_path):
    data_table = unreal.EditorAssetLibrary.load_asset(asset_path)
    if data_table is None and "." in asset_path:
        data_table = unreal.EditorAssetLibrary.load_asset(asset_path.split(".")[0])
    if data_table is None:
        fail("SONG_DATA_TABLE_NOT_FOUND", "Could not load DataTable asset {0}".format(asset_path))
    if not isinstance(data_table, unreal.DataTable):
        fail("SONG_DATA_TABLE_INVALID", "Asset {0} is not a DataTable.".format(asset_path))
    return data_table


def main():
    action = get_arg("--mm-action", os.environ.get("MM_SONGDATA_ACTION", ""))
    table_path = get_arg("--mm-table", os.environ.get("MM_SONGDATA_TABLE", "/Game/Data/SongData.SongData"))
    csv_path = os.path.abspath(get_arg("--mm-csv", os.environ.get("MM_SONGDATA_CSV", "")))

    if action not in ("export", "import"):
        fail("INVALID_ACTION", "Expected --mm-action=export or --mm-action=import.")
    if not csv_path:
        fail("CSV_PATH_MISSING", "--mm-csv is required.")

    os.makedirs(os.path.dirname(csv_path), exist_ok=True)
    data_table = load_data_table(table_path)
    library = unreal.DataTableFunctionLibrary

    if action == "export":
        export_csv = require_function(library, "export_data_table_to_csv_file")
        ok = export_csv(data_table, csv_path)
        if ok is False:
            fail("SONG_DATA_TABLE_EXPORT_FAILED", "Unreal returned false while exporting {0}.".format(table_path))
        print(json.dumps({"ok": True, "action": action, "csvPath": csv_path, "assetPath": table_path}))
        return

    if not os.path.exists(csv_path):
        fail("CSV_PATH_NOT_FOUND", "Import CSV does not exist: {0}".format(csv_path))

    fill_csv = require_function(library, "fill_data_table_from_csv_file")
    ok = fill_csv(data_table, csv_path)
    if ok is False:
        fail("SONG_DATA_TABLE_IMPORT_FAILED", "Unreal returned false while importing {0}.".format(csv_path))

    unreal.EditorAssetLibrary.save_loaded_asset(data_table)
    print(json.dumps({"ok": True, "action": action, "csvPath": csv_path, "assetPath": table_path}))


try:
    main()
except SystemExit:
    raise
except Exception as exc:
    unreal.log_error(traceback.format_exc())
    fail("UNHANDLED_EXCEPTION", str(exc))
