import json
import unreal

out_path = r"C:\Users\Johan\source\repos\Unreal\MusicManager\Saved\TopStatusBarBP_api.json"
asset = unreal.EditorAssetLibrary.load_asset("/Game/GUI/HUD/TopStatusBarBP")

data = {
    "asset": str(asset),
    "class": asset.get_class().get_name() if asset else None,
    "dir": [],
    "props": {},
    "class_dir": [],
}

if asset:
    data["dir"] = sorted([name for name in dir(asset) if not name.startswith("__")])
    for prop in [
        "WidgetTree", "widget_tree", "GeneratedClass", "generated_class",
        "SkeletonGeneratedClass", "ParentClass", "parent_class", "BlueprintType",
        "BlueprintCategory", "bForceFullEditor",
    ]:
        try:
            value = asset.get_editor_property(prop)
            data["props"][prop] = str(value)
        except Exception as exc:
            data["props"][prop] = f"ERR: {exc}"

    cls = asset.get_class()
    data["class_dir"] = sorted([name for name in dir(cls) if not name.startswith("__")])

with open(out_path, "w", encoding="utf-8") as f:
    json.dump(data, f, indent=2)

unreal.log(f"Wrote {out_path}")
