"""区域守卫配置检查（只读，不修改任何资产）。

由 Scripts/Inspect-RegionGuards.ps1 调用，结果写入
Saved/Automation/RegionGuards/region_guards_report.json。
该报告目录被 .gitignore 忽略，重要结论请同步到 Docs/协作交接.md。

已知的 UE 5.7 Python 限制（本轮用一次性探针实测）：
  * UBehaviorTree::BlackboardAsset / RootNode 是私有 UPROPERTY，get_editor_property
    读不到；黑板要用 bt.get_blackboard_asset()，行为树节点树没有公开读取入口。
  * FBlackboardEntry::EntryName 用属性访问会得到空值，必须走 get_editor_property。
    因此本脚本核对黑板键与蓝图引用，行为树内部连线仍需在编辑器里人工确认。
"""

import json
import traceback
from pathlib import Path

import unreal

PROJECT_ROOT = Path(__file__).resolve().parent.parent
REPORT_PATH = PROJECT_ROOT / "Saved" / "Automation" / "RegionGuards" / "region_guards_report.json"

MELEE_BP = "/Game/MyStuff/BluePrints/Enemies/BP_RegionMeleeGuard"
RANGED_BP = "/Game/MyStuff/BluePrints/Enemies/BP_RegionRangedGuard"
ELITE_MELEE_BP = "/Game/MyStuff/BluePrints/Enemies/BP_ShooterEliteMeleeEnemy"

REGION_MELEE_BT = "/Game/MyStuff/EnemyBT/BT_RegionMeleeGuard"
REGION_RANGED_BT = "/Game/MyStuff/EnemyBT/BT_RegionRangedGuard"
REGION_BB = "/Game/MyStuff/EnemyBT/BB_RegionGuard"

REGION_MAP = "/Game/MyStuff/Maps/Escape"

REQUIRED_BLACKBOARD_KEYS = (
    "TargetActor",
    "HasLineOfSight",
    "IsInAttackRange",
    "GuardHomeLocation",
    "GuardReturning",
)

REPORT = {}
PROBLEMS = []


def check(condition, message):
    if not condition:
        PROBLEMS.append(message)
    return condition


def text(value):
    """Path name for a UE object, or a readable stand-in."""
    if value is None:
        return None
    try:
        return value.get_path_name()
    except Exception:
        return str(value)


def read(obj, name):
    """Editor property value, or None when it is missing/private.

    UE's Python bindings return a dict from get_editor_property when the
    attribute never existed on the reflected class.
    """
    if obj is None:
        return None
    try:
        value = obj.get_editor_property(name)
    except Exception:
        return None
    return None if isinstance(value, dict) else value


def safe_call(fn):
    try:
        return fn()
    except Exception:
        return None


def default_object(asset_path):
    """(asset, class default object) for a Blueprint asset."""
    asset = unreal.load_asset(asset_path)
    if asset is None:
        return None, None
    cls = safe_call(lambda: asset.generated_class())
    if cls is None:
        return asset, None
    return asset, safe_call(lambda: unreal.get_default_object(cls))


# --------------------------------------------------------------------------- 蓝图


def inspect_blueprint(asset_path, label, sheet="/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple"):
    entry = {"path": asset_path, "asset_loaded": False}
    asset, cdo = default_object(asset_path)

    if asset is None:
        check(False, "%s: 蓝图无法加载" % label)
        return entry

    entry["asset_loaded"] = True
    if cdo is None:
        check(False, "%s: 无法取得类默认对象" % label)
        return entry

    actor_class = cdo.get_class()
    entry["generated_class"] = text(actor_class)
    entry["parent_class"] = text(safe_call(lambda: actor_class.get_super_class()))
    entry["behavior_tree"] = text(read(cdo, "behavior_tree"))
    entry["max_health"] = read(cdo, "max_health")
    entry["melee_damage"] = read(cdo, "melee_damage")
    entry["melee_attack_range"] = read(cdo, "melee_attack_range")
    entry["kill_reward_coins"] = read(cdo, "kill_reward_coins")
    entry["death_lifespan"] = read(cdo, "death_lifespan")
    entry["ai_controller_class"] = text(read(cdo, "ai_controller_class"))

    mesh = read(cdo, "mesh")
    entry["skeletal_mesh"] = text(safe_call(lambda: mesh.get_skeletal_mesh_asset())) if mesh else None
    entry["anim_class"] = text(read(mesh, "anim_class")) if mesh else None

    if not entry["behavior_tree"]:
        check(False, "%s: 未设置 Behavior Tree" % label)
    if sheet and entry["skeletal_mesh"] and sheet not in entry["skeletal_mesh"]:
        check(False, "%s: 骨骼网格不是 %s（实际 %s）"
              % (label, sheet, entry["skeletal_mesh"]))
    if not entry["anim_class"]:
        check(False, "%s: SkeletalMeshComponent 没有 Anim Class，Anim Blueprint 不会生效" % label)

    return entry


def check_uses_tree(blueprint_entry, label, expected_tree_name):
    actual = blueprint_entry.get("behavior_tree")
    if not actual:
        return
    if expected_tree_name not in actual:
        check(False, "%s 的 Behavior Tree 是 %s，应改为 %s"
              % (label, actual, expected_tree_name))


# --------------------------------------------------------------------------- 行为树


def blackboard_entry_name(entry):
    """FBlackboardEntry::EntryName is an FName; attribute access can silently
    return an empty value, so read it through the editor property API."""
    value = read(entry, "entry_name")
    if value is None:
        value = read(entry, "EntryName")
    return str(value) if value is not None else None


def inspect_blackboard_asset(asset_path):
    """Read the keys of a Blackboard asset (worked around the private BT property)."""
    entry = {"path": asset_path, "keys": [], "parent": None}
    bb = unreal.load_asset(asset_path)
    if bb is None:
        check(False, "黑板无法加载: %s" % asset_path)
        return entry

    entry["parent"] = text(read(bb, "parent"))

    keys = read(bb, "keys")
    if keys is None:
        check(False, "%s: 读不到 Keys 数组" % asset_path)
        return entry

    for index in range(len(keys)):
        key = keys[index]
        name = blackboard_entry_name(key)
        entry["keys"].append({
            "name": name,
            "type": text(safe_call(lambda k=key: k.key_type)),
        })

    names = [k["name"] for k in entry["keys"]]
    if not names or names[0] is None:
        check(False, "%s: 黑板键名读取失败（%d 个元素）" % (asset_path, len(entry["keys"])))
        return entry

    for required in REQUIRED_BLACKBOARD_KEYS:
        check(required in names, "%s 缺少黑板键: %s（现有: %s）"
              % (asset_path, required, ", ".join(n for n in names if n)))

    return entry


def inspect_behavior_tree(asset_path):
    entry = {"path": asset_path, "loaded": False, "blackboard": None,
             "node_tree_readable": False}
    bt = unreal.load_asset(asset_path)
    if bt is None:
        check(False, "行为树无法加载: %s" % asset_path)
        return entry

    entry["loaded"] = True
    try:
        entry["blackboard"] = text(bt.get_blackboard_asset())
    except Exception as exc:
        entry["blackboard_error"] = str(exc)

    if not entry["blackboard"]:
        check(False, "%s: 未指定 Blackboard Asset" % asset_path)

    # RootNode 是私有属性，没有公开读取入口；节点连线在编辑器里人工核对。
    if read(bt, "root_node") is not None:
        entry["node_tree_readable"] = True

    return entry


def inspect_melee_attack_task(asset_path):
    """Melee attack options live on the BT node, which Python cannot reach.

    Recorded here so the report states the gap instead of silently passing.
    """
    return {
        "path": asset_path,
        "attack_options_readable_by_python": False,
        "note": "Attacks 数组在行为树节点上，当前只能用编辑器打开 Random Melee Attack 核对。",
    }


# --------------------------------------------------------------------------- 动画


def inspect_animation_blueprint(asset_path, expected_slot):
    """Check the slot by looking for it in the compiled Anim Blueprint class.

    AnimationLibrary.get_animation_slot_names is not available in this build, so the
    compiled generated class is scanned for the slot name instead.
    """
    entry = {"path": asset_path, "loaded": False, "slot_present": None}
    abp = unreal.load_asset(asset_path)
    if abp is None:
        check(False, "动画蓝图无法加载: %s" % asset_path)
        return entry

    entry["loaded"] = True
    generated_class = safe_call(lambda: abp.generated_class())
    if generated_class is None:
        check(False, "%s: 动画蓝图没有生成类，可能未编译" % asset_path)
        return entry

    haystack = " ".join(filter(None, (
        str(generated_class),
        str(safe_call(lambda: generated_class.get_path_name())),
        " ".join(str(name) for name in (safe_call(lambda: generated_class.get_editor_property("anim_node_properties")) or [])),
    )))

    entry["slot_present"] = expected_slot in haystack
    entry["scanned"] = "generated_class"
    if not entry["slot_present"]:
        # get_animation_slot_names 在该版本不可用，无法穷举，只能提示人工确认。
        entry["note"] = ("未在生成类中找到 %s；该版本无槽名枚举 API，"
                         "请在 Anim Blueprint 里人工确认存在该 Slot 节点。" % expected_slot)

    return entry


# --------------------------------------------------------------------------- 地图


def get_actor_label(actor):
    return str(safe_call(lambda: actor.get_actor_label()))


def get_actor_location(actor):
    value = safe_call(lambda: actor.get_actor_location())
    return [round(v, 1) for v in value.to_tuple()] if value is not None else None


def inspect_map(asset_path):
    entry = {"path": asset_path, "spawners": [], "target_points": [], "guards": [],
             "doors": [], "actor_class_names": {}}
    try:
        unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(asset_path)
    except Exception as exc:
        check(False, "地图加载失败 %s: %s" % (asset_path, exc))
        return entry

    for actor in unreal.EditorLevelLibrary.get_all_level_actors():
        cls = actor.get_class().get_name()
        entry["actor_class_names"][cls] = entry["actor_class_names"].get(cls, 0) + 1

        if cls == "RegionGuardSpawner":
            item = {"label": get_actor_label(actor)}
            item["region_id"] = str(read(actor, "region_id"))
            item["spawn_trigger"] = str(read(actor, "spawn_trigger"))
            item["guard_count"] = read(actor, "guard_count")
            item["ranged_guard_count"] = read(actor, "ranged_guard_count")
            item["melee_guard_class"] = text(read(actor, "melee_guard_class"))
            item["ranged_guard_class"] = text(read(actor, "ranged_guard_class"))

            points = []
            raw_points = read(actor, "spawn_points")
            if raw_points is not None:
                for index in range(len(raw_points)):
                    point = raw_points[index]
                    points.append(get_actor_label(point) if point else "None")
            item["spawn_points"] = points
            item["location"] = get_actor_location(actor)
            entry["spawners"].append(item)

            label = item["label"]
            check(item["region_id"] not in ("None", ""),
                  "生成器 %s 的 Region Id 为空，不会与任何区域门联动" % label)
            check(bool(item["melee_guard_class"]) and item["melee_guard_class"] != "None",
                  "生成器 %s 未设置 Melee Guard Class，近战位置会生成失败" % label)
            check(bool(item["ranged_guard_class"]) and item["ranged_guard_class"] != "None",
                  "生成器 %s 未设置 Ranged Guard Class" % label)

            count = item["guard_count"]
            if isinstance(count, int):
                check(count > 0, "生成器 %s 的 Guard Count 为 %d" % (label, count))
                check(len(points) >= count,
                      "生成器 %s: Spawn Points 只有 %d 个，少于 Guard Count %d"
                      % (label, len(points), count))

        elif cls == "TargetPoint":
            entry["target_points"].append({
                "label": get_actor_label(actor),
                "location": get_actor_location(actor),
            })

        elif "RegionDoor" in cls:
            entry["doors"].append({
                "label": get_actor_label(actor),
                "class": cls,
                "door_group_id": str(read(actor, "door_group_id")),
                "required_wave": read(actor, "required_wave"),
                "requires_power": read(actor, "requires_power"),
                "unlock_cost": read(actor, "unlock_cost"),
                "location": get_actor_location(actor),
            })

        elif "Enemy" in cls and "Spawner" not in cls:
            # 手动摆放在地图里的测试敌人（含从 C++ 类派生的蓝图子类）。
            entry["guards"].append({
                "label": get_actor_label(actor),
                "class": cls,
                "blueprint": text(safe_call(lambda a=actor: a.get_class().get_outer())),
                "location": get_actor_location(actor),
            })

    # 区域广播按 DoorGroupId 匹配：生成器的 Region Id 必须能在门里找到同组。
    door_groups = {d["door_group_id"] for d in entry["doors"]}
    for spawner in entry["spawners"]:
        region_id = spawner["region_id"]
        if region_id in ("None", ""):
            continue
        if region_id not in door_groups:
            check(False, "生成器 %s 的 Region Id=%s 在门里找不到同组 Door Group Id（门组: %s）"
                  % (spawner["label"], region_id,
                     ", ".join(sorted(door_groups - {"None"})) or "无"))

    if not entry["spawners"]:
        check(False, "%s: 没有找到 RegionGuardSpawner" % asset_path)

    return entry


# --------------------------------------------------------------------------- 主流程


def main():
    REPORT["blueprints"] = {
        # 近战区域守卫复用精英近战蓝图的 Quinn 骨骼与动画蓝图。
        "region_melee": inspect_blueprint(MELEE_BP, "BP_RegionMeleeGuard"),
        # 远程守卫用 Wraith 骨骼，不检查 Quinn。
        "region_ranged": inspect_blueprint(RANGED_BP, "BP_RegionRangedGuard", sheet=None),
        "elite_melee": inspect_blueprint(ELITE_MELEE_BP, "BP_ShooterEliteMeleeEnemy"),
    }
    check_uses_tree(REPORT["blueprints"]["region_melee"],
                    "BP_RegionMeleeGuard", "BT_RegionMeleeGuard")
    check_uses_tree(REPORT["blueprints"]["region_ranged"],
                    "BP_RegionRangedGuard", "BT_RegionRangedGuard")

    REPORT["blackboard"] = inspect_blackboard_asset(REGION_BB)
    REPORT["behavior_trees"] = {
        "region_melee": inspect_behavior_tree(REGION_MELEE_BT),
        "region_ranged": inspect_behavior_tree(REGION_RANGED_BT),
        "elite_melee": inspect_behavior_tree("/Game/MyStuff/EnemyBT/BT_ShooterEliteMeleeEnemy"),
    }
    REPORT["melee_attack_task"] = inspect_melee_attack_task(REGION_MELEE_BT)
    REPORT["animation"] = inspect_animation_blueprint(
        "/Game/MyStuff/Animation/ABP_ShooterMeleeEnemy", "DefaultSlot")
    REPORT["map"] = inspect_map(REGION_MAP)


if __name__ == "__main__":
    try:
        main()
        REPORT["success"] = True
    except Exception:
        REPORT["success"] = False
        REPORT["error"] = traceback.format_exc()

    REPORT["problems"] = PROBLEMS
    REPORT_PATH.write_text(json.dumps(REPORT, indent=2, ensure_ascii=False, default=str),
                           encoding="utf-8")

    unreal.log("REGIONGUARD_CHECK_BEGIN")
    for section in ("blueprints", "behavior_trees", "animation", "map"):
        unreal.log("REGIONGUARD_SECTION %s: %s"
                   % (section, json.dumps(REPORT.get(section, {}), ensure_ascii=False, default=str)))

    if not REPORT.get("success"):
        unreal.log_error("REGIONGUARD_RESULT FAILED_TO_RUN")
    elif PROBLEMS:
        unreal.log_error("REGIONGUARD_RESULT PROBLEMS=%d" % len(PROBLEMS))
    else:
        unreal.log("REGIONGUARD_RESULT OK")

    for problem in PROBLEMS:
        unreal.log_error("REGIONGUARD_PROBLEM " + problem)

    unreal.log("REGIONGUARD_CHECK_END report=%s" % REPORT_PATH)
