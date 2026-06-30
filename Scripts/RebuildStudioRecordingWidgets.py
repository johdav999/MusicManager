import os
import unreal


PROJECT_CONTENT = os.path.join(unreal.Paths.project_content_dir(), "GUI", "StudioRecording")


def import_texture(filename, destination_path="/Game/GUI/StudioRecording"):
    source = os.path.join(PROJECT_CONTENT, filename)
    if not os.path.exists(source):
        raise RuntimeError(f"Missing studio recording icon source: {source}")

    asset_name = os.path.splitext(filename)[0]
    destination_asset = f"{destination_path}/{asset_name}"
    if unreal.EditorAssetLibrary.does_asset_exist(destination_asset):
        return

    task = unreal.AssetImportTask()
    task.filename = source
    task.destination_path = destination_path
    task.destination_name = asset_name
    task.automated = True
    task.save = True
    task.replace_existing = True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    if not unreal.EditorAssetLibrary.does_asset_exist(destination_asset):
        raise RuntimeError(f"Failed to import studio recording texture: {destination_asset}")


def import_studio_icons():
    for filename in (
        "StudioRecordingIcon_Clock.png",
        "StudioRecordingIcon_Warning.png",
        "StudioRecordingIcon_Filter.png",
        "StudioRecordingIcon_Search.png",
    ):
        import_texture(filename)


def delete_stale_row_blueprints():
    for stale_asset in (
        "/Game/GUI/RecordSongListBP",
        "/Game/GUI/RecordingRecordListBP",
    ):
        if unreal.EditorAssetLibrary.does_asset_exist(stale_asset):
            unreal.EditorAssetLibrary.delete_asset(stale_asset)


def rebuild_blueprints():
    import_studio_icons()

    result = unreal.MusicManagerWidgetBlueprintTools.rebuild_studio_recording_blueprints()
    if not result:
        raise RuntimeError("Studio recording Blueprint rebuild failed.")

    for asset_path in (
        "/Game/GUI/RecordingGUIBP",
        "/Game/GUI/RecordSongListBP",
        "/Game/GUI/RecordingRecordListBP",
        "/Game/GUI/StudioRecording/StudioRecordingIcon_Clock",
        "/Game/GUI/StudioRecording/StudioRecordingIcon_Warning",
        "/Game/GUI/StudioRecording/StudioRecordingIcon_Filter",
        "/Game/GUI/StudioRecording/StudioRecordingIcon_Search",
    ):
        unreal.EditorAssetLibrary.save_asset(asset_path, only_if_is_dirty=False)

    unreal.log("RebuildStudioRecordingWidgets.py completed.")


rebuild_blueprints()