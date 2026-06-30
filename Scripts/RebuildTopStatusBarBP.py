import unreal


PROJECT_HUD_DIR = "/Game/GUI/HUD"
SOURCE_DIR = r"C:\Users\Johan\source\repos\Unreal\MusicManager\Content\GUI\HUD"

TEXTURES = [
    "TopStatusBarSurface",
    "TopStatusBarFrame",
    "TopStatusBarIconsSheet",
    "TopStatusIcon_BrandRecord",
    "TopStatusIcon_DateCalendar",
    "TopStatusIcon_LabelPerson",
    "TopStatusIcon_CashDollar",
    "TopStatusIcon_ReputationStar",
    "TopStatusIcon_Pause",
    "TopStatusIcon_Play",
    "TopStatusIcon_FastForward",
    "TopStatusIcon_Menu",
]


def import_textures():
    tasks = []
    for texture_name in TEXTURES:
        task = unreal.AssetImportTask()
        task.filename = f"{SOURCE_DIR}\\{texture_name}.png"
        task.destination_path = PROJECT_HUD_DIR
        task.destination_name = texture_name
        task.automated = True
        task.replace_existing = True
        task.save = True
        tasks.append(task)

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

    for texture_name in TEXTURES:
        asset_path = f"{PROJECT_HUD_DIR}/{texture_name}.{texture_name}"
        if unreal.load_asset(asset_path) is None:
            raise RuntimeError(f"Texture import failed: {asset_path}")


def rebuild_blueprint():
    result = unreal.MusicManagerWidgetBlueprintTools.rebuild_top_status_bar_blueprint()
    if not result:
        raise RuntimeError("TopStatusBarBP rebuild failed.")


import_textures()
rebuild_blueprint()
unreal.EditorAssetLibrary.save_directory(PROJECT_HUD_DIR, only_if_is_dirty=False, recursive=True)
unreal.log("RebuildTopStatusBarBP.py completed.")
