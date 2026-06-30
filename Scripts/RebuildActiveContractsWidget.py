import os
import unreal


PROJECT_CONTENT = os.path.join(unreal.Paths.project_content_dir(), "GUI", "Contracts")
DESTINATION_PATH = "/Game/GUI/Contracts"


def import_texture(filename):
    source = os.path.join(PROJECT_CONTENT, filename)
    if not os.path.exists(source):
        raise RuntimeError(f"Missing active contracts texture source: {source}")

    asset_name = os.path.splitext(filename)[0]
    destination_asset = f"{DESTINATION_PATH}/{asset_name}"

    task = unreal.AssetImportTask()
    task.filename = source
    task.destination_path = DESTINATION_PATH
    task.destination_name = asset_name
    task.automated = True
    task.save = True
    task.replace_existing = True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    if not unreal.EditorAssetLibrary.does_asset_exist(destination_asset):
        raise RuntimeError(f"Failed to import active contracts texture: {destination_asset}")


def import_contract_icons():
    for filename in (
        "ActiveContractsIconSheet_OpenAIImage2.png",
        "ActiveContractsIcon_Artist.png",
        "ActiveContractsIcon_Bonus.png",
        "ActiveContractsIcon_Calendar.png",
        "ActiveContractsIcon_Close.png",
        "ActiveContractsIcon_Coins.png",
        "ActiveContractsIcon_Contract.png",
        "ActiveContractsIcon_Handshake.png",
        "ActiveContractsIcon_Record.png",
        "ActiveContractsIcon_Revenue.png",
        "ActiveContractsIcon_Royalty.png",
        "ActiveContractsIcon_Status.png",
    ):
        import_texture(filename)


def rebuild_blueprints():
    import_contract_icons()

    result = unreal.MusicManagerWidgetBlueprintTools.rebuild_active_contracts_blueprints()
    if not result:
        raise RuntimeError("Active contracts Blueprint rebuild failed.")

    for asset_path in (
        "/Game/GUI/Contracts/ActiveContractsBP",
        "/Game/GUI/Contracts/ActiveContractItemBP",
        "/Game/GUI/Contracts/ActiveContractsIconSheet_OpenAIImage2",
        "/Game/GUI/Contracts/ActiveContractsIcon_Artist",
        "/Game/GUI/Contracts/ActiveContractsIcon_Bonus",
        "/Game/GUI/Contracts/ActiveContractsIcon_Calendar",
        "/Game/GUI/Contracts/ActiveContractsIcon_Close",
        "/Game/GUI/Contracts/ActiveContractsIcon_Coins",
        "/Game/GUI/Contracts/ActiveContractsIcon_Contract",
        "/Game/GUI/Contracts/ActiveContractsIcon_Handshake",
        "/Game/GUI/Contracts/ActiveContractsIcon_Record",
        "/Game/GUI/Contracts/ActiveContractsIcon_Revenue",
        "/Game/GUI/Contracts/ActiveContractsIcon_Royalty",
        "/Game/GUI/Contracts/ActiveContractsIcon_Status",
    ):
        unreal.EditorAssetLibrary.save_asset(asset_path, only_if_is_dirty=False)

    unreal.log("RebuildActiveContractsWidget.py completed.")


rebuild_blueprints()
