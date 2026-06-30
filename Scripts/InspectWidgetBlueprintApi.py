import unreal


asset = unreal.EditorAssetLibrary.load_asset("/Game/GUI/HUD/TopStatusBarBP")
unreal.log(f"asset={asset}")
unreal.log(f"asset class={asset.get_class().get_name() if asset else None}")

if asset:
    unreal.log(f"asset dir sample={[name for name in dir(asset) if 'class' in name.lower() or 'tree' in name.lower() or 'widget' in name.lower() or 'blueprint' in name.lower()]}")
    for prop in ["widget_tree", "generated_class", "parent_class"]:
        try:
            unreal.log(f"{prop}={asset.get_editor_property(prop)}")
        except Exception as exc:
            unreal.log_warning(f"Could not read {prop}: {exc}")

    try:
        tree = asset.get_editor_property("widget_tree")
        unreal.log(f"tree={tree}")
        unreal.log(f"tree class={tree.get_class().get_name() if tree else None}")
        unreal.log(f"tree methods={[name for name in dir(tree) if 'widget' in name.lower() or 'root' in name.lower()]}")
    except Exception as exc:
        unreal.log_warning(f"Widget tree inspection failed: {exc}")
