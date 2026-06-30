import unreal


PROJECT_AUDITION_DIR = "/Game/GUI/Audition"
SOURCE_DIR = r"C:\Users\Johan\source\repos\Unreal\MusicManager\Content\GUI\Audition"

TEXTURES = [
    "ArtistAuditionReferenceSource",
    "ArtistAuditionPanelSurface",
    "ArtistAuditionContractSurface",
    "ArtistAuditionVinylFrame",
    "ArtistAuditionDefaultPortrait",
    "ArtistAuditionSignButtonSurface",
    "ArtistAuditionPassButtonSurface",
    "ArtistAuditionSliderTrackSurface",
    "ArtistAuditionSliderThumbSurface",
    "ArtistAuditionMeterSegmentSurface",
    "ArtistAuditionIcon_Record",
    "ArtistAuditionIcon_Location",
    "ArtistAuditionIcon_Performance",
    "ArtistAuditionIcon_Stage",
    "ArtistAuditionIcon_Audience",
    "ArtistAuditionIcon_Vocal",
    "ArtistAuditionIcon_Songwriting",
    "ArtistAuditionIcon_Contract",
    "ArtistAuditionIcon_Gift",
    "ArtistAuditionIcon_PlusPerson",
    "ArtistAuditionIcon_PassArrow",
]

def import_textures():
    tasks = []
    for texture_name in TEXTURES:
        task = unreal.AssetImportTask()
        task.filename = f"{SOURCE_DIR}\\{texture_name}.png"
        task.destination_path = PROJECT_AUDITION_DIR
        task.destination_name = texture_name
        task.automated = True
        task.replace_existing = True
        task.save = True
        tasks.append(task)

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

    for texture_name in TEXTURES:
        asset_path = f"{PROJECT_AUDITION_DIR}/{texture_name}.{texture_name}"
        if unreal.load_asset(asset_path) is None:
            raise RuntimeError(f"Texture import failed: {asset_path}")


def rebuild_blueprint():
    result = unreal.MusicManagerWidgetBlueprintTools.rebuild_artist_audition_panel_blueprint()
    if not result:
        raise RuntimeError("Artist audition panel Blueprint rebuild failed.")


import_textures()
rebuild_blueprint()
unreal.EditorAssetLibrary.save_directory(PROJECT_AUDITION_DIR, only_if_is_dirty=False, recursive=True)
unreal.EditorAssetLibrary.save_asset("/Game/GUI/Audition/ArtistAuditionPanelBP", only_if_is_dirty=False)
unreal.log("RebuildArtistAuditionPanel.py completed.")
