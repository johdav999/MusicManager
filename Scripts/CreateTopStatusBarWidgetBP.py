import unreal


ASSET_DIR = "/Game/GUI/HUD"
ASSET_NAME = "TopStatusBarBP"
ASSET_PATH = f"{ASSET_DIR}/{ASSET_NAME}"


def log(message):
    unreal.log(f"[CreateTopStatusBarWidgetBP] {message}")


def main():
    editor_assets = unreal.EditorAssetLibrary
    if not editor_assets.does_directory_exist(ASSET_DIR):
        editor_assets.make_directory(ASSET_DIR)
        log(f"Created asset directory {ASSET_DIR}")

    if editor_assets.does_asset_exist(ASSET_PATH):
        existing = editor_assets.load_asset(ASSET_PATH)
        log(f"Asset already exists: {ASSET_PATH}")
        if existing:
            editor_assets.save_loaded_asset(existing)
        return

    parent_class = unreal.TopStatusBarWidget.static_class()
    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property("parent_class", parent_class)

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    asset = asset_tools.create_asset(
        asset_name=ASSET_NAME,
        package_path=ASSET_DIR,
        asset_class=unreal.WidgetBlueprint,
        factory=factory,
    )

    if not asset:
        raise RuntimeError(f"Failed to create widget blueprint at {ASSET_PATH}")

    editor_assets.save_loaded_asset(asset)
    log(f"Created {ASSET_PATH} inheriting {parent_class.get_name()}")


main()
