import os
import unreal


PROJECT_DIR = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SOURCE_DIR = os.path.join(PROJECT_DIR, "Content", "GUI", "HUD", "CommandDock")
DESTINATION_PATH = "/Game/GUI/HUD/CommandDock"

TEXTURES = [
    "BottomCommandDock_Background.png",
    "BottomCommandDock_Button.png",
    "BottomCommandDockIcon_Audition.png",
    "BottomCommandDockIcon_Market.png",
    "BottomCommandDockIcon_Contracts.png",
    "BottomCommandDockIcon_Studio.png",
    "BottomCommandDockIcon_Charts.png",
]


def import_texture(filename):
    source_path = os.path.join(SOURCE_DIR, filename)
    if not os.path.exists(source_path):
        raise RuntimeError(f"Missing command dock texture source: {source_path}")

    task = unreal.AssetImportTask()
    task.filename = source_path
    task.destination_path = DESTINATION_PATH
    task.destination_name = os.path.splitext(filename)[0]
    task.replace_existing = True
    task.automated = True
    task.save = True

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    if not task.imported_object_paths:
        raise RuntimeError(f"Unreal did not import {source_path}")

    asset = unreal.load_asset(task.imported_object_paths[0])
    if asset:
        asset.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
        asset.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
        asset.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
        unreal.EditorAssetLibrary.save_loaded_asset(asset)

    return task.imported_object_paths[0]


def main():
    unreal.EditorAssetLibrary.make_directory(DESTINATION_PATH)

    imported = [import_texture(filename) for filename in TEXTURES]
    unreal.log(f"Imported bottom command dock textures: {imported}")

    if not unreal.MusicManagerWidgetBlueprintTools.rebuild_bottom_command_dock_blueprints():
        raise RuntimeError("Bottom command dock Blueprint rebuild failed.")

    unreal.EditorAssetLibrary.save_directory(DESTINATION_PATH, only_if_is_dirty=False, recursive=False)
    unreal.EditorAssetLibrary.save_asset("/Game/GUI/CommandPanelBP", only_if_is_dirty=False)
    unreal.EditorAssetLibrary.save_asset("/Game/GUI/CommandItemWidgetBP", only_if_is_dirty=False)
    unreal.log("Bottom command dock rebuild completed.")


main()
