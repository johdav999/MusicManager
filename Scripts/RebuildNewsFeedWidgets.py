import unreal


PROJECT_NEWS_DIR = "/Game/GUI/News"
SOURCE_DIR = r"C:\Users\Johan\source\repos\Unreal\MusicManager\Content\GUI\News"

TEXTURES = [
    "NewsFeedShellStyleSource",
    "NewsFeedIconsSheet",
    "NewsFeedPanelSurface",
    "NewsFeedCardSurface",
    "NewsFeedButtonSurface",
    "NewsFeedIcon_Microphone",
    "NewsFeedIcon_Radio",
    "NewsFeedIcon_Handshake",
    "NewsFeedIcon_Chart",
    "NewsFeedIcon_Calendar",
    "NewsFeedIcon_List",
    "NewsFeedIcon_ChevronRight",
]

def import_textures():
    tasks = []
    for texture_name in TEXTURES:
        task = unreal.AssetImportTask()
        task.filename = f"{SOURCE_DIR}\\{texture_name}.png"
        task.destination_path = PROJECT_NEWS_DIR
        task.destination_name = texture_name
        task.automated = True
        task.replace_existing = True
        task.save = True
        tasks.append(task)

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

    for texture_name in TEXTURES:
        asset_path = f"{PROJECT_NEWS_DIR}/{texture_name}.{texture_name}"
        if unreal.load_asset(asset_path) is None:
            raise RuntimeError(f"Texture import failed: {asset_path}")


def rebuild_blueprints():
    result = unreal.MusicManagerWidgetBlueprintTools.rebuild_news_feed_blueprints()
    if not result:
        raise RuntimeError("NewsFeed Blueprint rebuild failed.")


import_textures()
rebuild_blueprints()
unreal.EditorAssetLibrary.save_directory(PROJECT_NEWS_DIR, only_if_is_dirty=False, recursive=True)
unreal.EditorAssetLibrary.save_asset("/Game/GUI/NewFeedListBP", only_if_is_dirty=False)
unreal.EditorAssetLibrary.save_asset("/Game/GUI/NewsFeedItemBP", only_if_is_dirty=False)
unreal.log("RebuildNewsFeedWidgets.py completed.")
