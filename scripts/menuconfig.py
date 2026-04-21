#!/usr/bin/env python3
"""Pure CLI menuconfig for XiaoBaiOS.

This tool reads `configs/menuconfig/clks_features.json`, lets you edit the
feature values in a terminal, then writes:
  - `configs/menuconfig/.config.json`
  - `configs/menuconfig/config.cmake`

The implementation is intentionally text-only and does not depend on Qt.
"""

from __future__ import annotations

import argparse
import locale
import os
import json
import re
import sys
import textwrap
import unicodedata
from collections import OrderedDict
from dataclasses import dataclass
from pathlib import Path
from typing import Callable, Dict, Iterable, List, Sequence, Tuple

try:
    import curses
except Exception:
    curses = None


ROOT_DIR = Path(__file__).resolve().parent.parent
MENUCONFIG_DIR = ROOT_DIR / "configs" / "menuconfig"
CLKS_FEATURES_PATH = MENUCONFIG_DIR / "clks_features.json"
CONFIG_JSON_PATH = MENUCONFIG_DIR / ".config.json"
CONFIG_CMAKE_PATH = MENUCONFIG_DIR / "config.cmake"
SOURCE_NAMESPACE = "CLEONOS_"
TARGET_NAMESPACE = "XIAOBAIOS_"

TRI_N = 0
TRI_M = 1
TRI_Y = 2


@dataclass(frozen=True)
class Feature:
    key: str
    title: str
    description: str
    kind: str
    default: int
    depends_on: str
    selects: Tuple[str, ...]
    implies: Tuple[str, ...]
    group: str


@dataclass
class EvalResult:
    effective: Dict[str, int]
    requested: Dict[str, int]
    visible: Dict[str, bool]
    dep_level: Dict[str, int]
    min_required: Dict[str, int]
    max_allowed: Dict[str, int]


def tri_char(value: int) -> str:
    if value >= TRI_Y:
        return "y"
    if value >= TRI_M:
        return "m"
    return "n"


def tri_word(value: int) -> str:
    if value >= TRI_Y:
        return "enabled"
    if value >= TRI_M:
        return "module"
    return "disabled"


def normalize_kind(raw: object) -> str:
    text = str(raw or "bool").strip().lower()
    if text in {"tristate", "tri"}:
        return "tristate"
    return "bool"


def normalize_tri(raw: object, default: int, kind: str) -> int:
    if isinstance(raw, bool):
        value = TRI_Y if raw else TRI_N
    elif isinstance(raw, (int, float)):
        iv = int(raw)
        value = TRI_N if iv <= 0 else TRI_M if iv == 1 else TRI_Y
    elif isinstance(raw, str):
        text = raw.strip().lower()
        if text in {"1", "y", "yes", "true", "on"}:
            value = TRI_Y
        elif text in {"m", "module"}:
            value = TRI_M
        elif text in {"0", "n", "no", "false", "off"}:
            value = TRI_N
        else:
            value = default
    else:
        value = default

    if kind == "bool":
        return TRI_Y if value > TRI_N else TRI_N
    return TRI_N if value <= TRI_N else TRI_M if value == TRI_M else TRI_Y


def _normalize_key_list(raw: object) -> Tuple[str, ...]:
    if raw is None:
        return ()
    if isinstance(raw, str):
        raw = [raw]
    items: List[str] = []
    for item in raw:
        token = str(item).strip()
        if token:
            items.append(token)
    return tuple(items)


def remap_namespace(name: str) -> str:
    text = str(name).strip()
    if text.startswith(SOURCE_NAMESPACE):
        return TARGET_NAMESPACE + text[len(SOURCE_NAMESPACE):]
    return text


def remap_text(value: object) -> str:
    text = str(value or "")
    return text.replace(SOURCE_NAMESPACE, TARGET_NAMESPACE)


TEXTS = {
    "en": {
        "title": "XiaoBaiOS menuconfig",
        "group_pane": "Groups",
        "option_pane": "Options",
        "detail_pane": "Details",
        "footer": "↑↓ move  ←→/Tab switch  Space/Enter toggle  y/n/m/d set  s save  q quit",
        "saved": "menuconfig: saved",
        "no_changes": "menuconfig: no changes saved",
        "status": "Status",
        "focus_groups": "groups",
        "focus_options": "options",
        "lang": "Language",
        "enabled": "enabled",
        "module": "module",
        "disabled": "disabled",
        "requested": "requested",
        "effective": "effective",
        "selected": "selected",
        "forced": "forced",
        "locked": "locked",
        "hidden": "hidden",
        "help": "Help",
        "group_count": "{enabled}/{total}",
        "feature_count": "{enabled}/{total} enabled",
        "current_group": "Current group",
        "current_option": "Current option",
    },
    "zh": {
        "title": "XiaoBaiOS 菜单配置",
        "group_pane": "分组",
        "option_pane": "选项",
        "detail_pane": "详情",
        "footer": "↑↓ 移动  ←→/Tab 切换  空格/回车 切换  y/n/m/d 设置  s 保存  q 退出",
        "saved": "menuconfig：已保存",
        "no_changes": "menuconfig：未保存任何更改",
        "status": "状态",
        "focus_groups": "分组",
        "focus_options": "选项",
        "lang": "语言",
        "enabled": "启用",
        "module": "模块",
        "disabled": "禁用",
        "requested": "请求值",
        "effective": "生效值",
        "selected": "被选择",
        "forced": "被强制",
        "locked": "已锁定",
        "hidden": "隐藏",
        "help": "帮助",
        "group_count": "{enabled}/{total}",
        "feature_count": "已启用 {enabled}/{total}",
        "current_group": "当前分组",
        "current_option": "当前选项",
    },
}

GROUP_LABELS = {
    "General": {"en": "General", "zh": "通用"},
    "USC Syscall Policy": {"en": "USC Syscall Policy", "zh": "USC 系统调用策略"},
}


def detect_language(requested: str) -> str:
    req = (requested or "auto").strip().lower()
    if req in {"en", "zh"}:
        return req

    env_text = " ".join(
        [
            os.environ.get("LC_ALL", ""),
            os.environ.get("LC_MESSAGES", ""),
            os.environ.get("LANG", ""),
            os.environ.get("LANGUAGE", ""),
        ]
    ).lower()
    if "zh" in env_text or "cn" in env_text:
        return "zh"
    return "en"


def tr(lang: str, key: str, **kwargs: object) -> str:
    base = TEXTS.get(lang, TEXTS["en"]).get(key, key)
    return str(base).format(**kwargs)


def translate_group_name(name: str, lang: str) -> str:
    entry = GROUP_LABELS.get(name)
    if entry is None:
        return name
    return entry.get(lang, entry["en"])


def display_width(text: str) -> int:
    width = 0
    for ch in text:
        if unicodedata.east_asian_width(ch) in {"F", "W"}:
            width += 2
        else:
            width += 1
    return width


def truncate_text(text: str, max_width: int) -> str:
    if max_width <= 0:
        return ""
    if display_width(text) <= max_width:
        return text

    result: List[str] = []
    used = 0
    for ch in text:
        ch_width = 2 if unicodedata.east_asian_width(ch) in {"F", "W"} else 1
        if used + ch_width > max_width - 1:
            break
        result.append(ch)
        used += ch_width
    result.append("…")
    return "".join(result)


def pad_text(text: str, width: int) -> str:
    clipped = truncate_text(text, width)
    padding = width - display_width(clipped)
    if padding > 0:
        clipped += " " * padding
    return clipped


def load_features() -> List[Feature]:
    if not CLKS_FEATURES_PATH.exists():
        raise RuntimeError(f"missing feature file: {CLKS_FEATURES_PATH}")

    raw = json.loads(CLKS_FEATURES_PATH.read_text(encoding="utf-8"))
    if not isinstance(raw, dict) or not isinstance(raw.get("features"), list):
        raise RuntimeError(f"invalid feature file: {CLKS_FEATURES_PATH}")

    features: List[Feature] = []
    for entry in raw["features"]:
        if not isinstance(entry, dict):
            continue
        key = str(entry.get("key", "")).strip()
        if not key:
            continue
        kind = normalize_kind(entry.get("type", "bool"))
        features.append(
            Feature(
                key=remap_namespace(key),
                title=str(entry.get("title", key)).strip() or key,
                description=remap_text(entry.get("description", "")).strip(),
                kind=kind,
                default=normalize_tri(entry.get("default", TRI_N), TRI_Y, kind),
                depends_on=remap_namespace(str(entry.get("depends_on", entry.get("depends", ""))).strip()),
                selects=tuple(remap_namespace(token) for token in _normalize_key_list(entry.get("select", entry.get("selects")))),
                implies=tuple(remap_namespace(token) for token in _normalize_key_list(entry.get("imply", entry.get("implies")))),
                group=str(entry.get("group", entry.get("menu", "General"))).strip() or "General",
            )
        )

    if not features:
        raise RuntimeError(f"no features found in {CLKS_FEATURES_PATH}")
    return features


def load_previous_values() -> Dict[str, int]:
    if not CONFIG_JSON_PATH.exists():
        return {}
    try:
        raw = json.loads(CONFIG_JSON_PATH.read_text(encoding="utf-8"))
    except Exception:
        return {}
    if not isinstance(raw, dict):
        return {}
    out: Dict[str, int] = {}
    for key, value in raw.items():
        if not isinstance(key, str):
            continue
        out[remap_namespace(key)] = normalize_tri(value, TRI_N, "tristate")
    return out


TOKEN_RE = re.compile(r"\s*(&&|\|\||!|\(|\)|[A-Za-z_][A-Za-z0-9_]*)")


def eval_dep_expr(expr: str, lookup: Callable[[str], int]) -> int:
    text = expr.strip()
    if not text:
        return TRI_Y

    tokens: List[str] = []
    pos = 0
    while pos < len(text):
        match = TOKEN_RE.match(text, pos)
        if not match:
            raise RuntimeError(f"invalid dependency expression: {expr!r}")
        token = match.group(1)
        tokens.append(token)
        pos = match.end()

    index = 0

    def peek() -> str:
        return tokens[index] if index < len(tokens) else ""

    def consume(expected: str = "") -> str:
        nonlocal index
        if index >= len(tokens):
            raise RuntimeError(f"unexpected end of dependency expression: {expr!r}")
        token = tokens[index]
        if expected and token != expected:
            raise RuntimeError(f"expected {expected!r} in dependency expression: {expr!r}")
        index += 1
        return token

    def parse_primary() -> int:
        token = peek()
        if token == "(":
            consume("(")
            value = parse_or()
            consume(")")
            return value
        if token and re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", token):
            consume()
            return lookup(token)
        raise RuntimeError(f"invalid token in dependency expression: {expr!r}")

    def parse_unary() -> int:
        if peek() == "!":
            consume("!")
            value = parse_unary()
            if value <= TRI_N:
                return TRI_Y
            if value >= TRI_Y:
                return TRI_N
            return TRI_M
        return parse_primary()

    def parse_and() -> int:
        value = parse_unary()
        while peek() == "&&":
            consume("&&")
            value = min(value, parse_unary())
        return value

    def parse_or() -> int:
        value = parse_and()
        while peek() == "||":
            consume("||")
            value = max(value, parse_and())
        return value

    result = parse_or()
    if index != len(tokens):
        raise RuntimeError(f"trailing tokens in dependency expression: {expr!r}")
    return result


def _select_floor(src_value: int, dst_kind: str, is_imply: bool) -> int:
    if src_value <= TRI_N:
        return TRI_N
    if dst_kind == "bool":
        if is_imply and src_value == TRI_M:
            return TRI_N
        return TRI_Y
    return src_value


def evaluate(features: Sequence[Feature], requested: Dict[str, int]) -> EvalResult:
    feature_index = {item.key: item for item in features}
    current: Dict[str, int] = {}
    for item in features:
        current[item.key] = normalize_tri(requested.get(item.key, item.default), item.default, item.kind)

    max_rounds = max(1, len(features) * 8)
    dep_level: Dict[str, int] = {}
    visible: Dict[str, bool] = {}
    min_required: Dict[str, int] = {}
    max_allowed: Dict[str, int] = {}

    for _ in range(max_rounds):
        for item in features:
            def _lookup(symbol: str) -> int:
                return current.get(symbol, TRI_N)

            try:
                dep_level[item.key] = eval_dep_expr(item.depends_on, _lookup)
            except Exception:
                dep_level[item.key] = TRI_N

        visible = {item.key: dep_level[item.key] > TRI_N for item in features}
        max_allowed = {}
        for item in features:
            dep = dep_level[item.key]
            if item.kind == "bool":
                max_allowed[item.key] = TRI_Y if dep >= TRI_Y else TRI_N
            else:
                max_allowed[item.key] = dep

        min_required = {item.key: TRI_N for item in features}
        for src in features:
            src_value = current.get(src.key, TRI_N)
            if src_value <= TRI_N:
                continue
            for dst_key in src.selects:
                dst = feature_index.get(dst_key)
                if dst is None:
                    continue
                floor = _select_floor(src_value, dst.kind, is_imply=False)
                if floor > min_required[dst_key]:
                    min_required[dst_key] = floor
            for dst_key in src.implies:
                dst = feature_index.get(dst_key)
                if dst is None:
                    continue
                floor = _select_floor(src_value, dst.kind, is_imply=True)
                if floor > min_required[dst_key]:
                    min_required[dst_key] = floor

        next_current: Dict[str, int] = {}
        changed = False
        for item in features:
            req = normalize_tri(requested.get(item.key, item.default), item.default, item.kind)
            floor = min_required[item.key]
            ceil = max_allowed[item.key]
            if floor > ceil:
                ceil = floor

            if item.kind == "bool":
                eff = TRI_Y if req > TRI_N else TRI_N
                if ceil <= TRI_N:
                    eff = TRI_N
                elif floor > TRI_N:
                    eff = TRI_Y
            else:
                eff = req
                if eff < floor:
                    eff = floor
                if eff > ceil:
                    eff = ceil

            next_current[item.key] = eff
            if eff != current.get(item.key, TRI_N):
                changed = True

        current = next_current
        if not changed:
            break

    return EvalResult(
        effective=current,
        requested={item.key: normalize_tri(requested.get(item.key, item.default), item.default, item.kind) for item in features},
        visible=visible,
        dep_level=dep_level,
        min_required=min_required,
        max_allowed=max_allowed,
    )


def group_summary(item: Sequence[Feature], ev: EvalResult) -> Tuple[int, int]:
    enabled = sum(1 for feature in item if ev.effective.get(feature.key, feature.default) > TRI_N)
    return enabled, len(item)


def grouped_features(features: Sequence[Feature]) -> List[Tuple[str, List[Feature]]]:
    groups: "OrderedDict[str, List[Feature]]" = OrderedDict()
    for item in features:
        groups.setdefault(item.group, []).append(item)
    return list(groups.items())


def option_state(item: Feature, ev: EvalResult) -> str:
    req = ev.requested.get(item.key, item.default)
    eff = ev.effective.get(item.key, item.default)
    flags: List[str] = []
    if not ev.visible.get(item.key, True):
        flags.append("hidden")
    if ev.min_required.get(item.key, TRI_N) > TRI_N:
        flags.append("forced")
    if ev.max_allowed.get(item.key, TRI_Y) <= TRI_N:
        flags.append("locked")
    suffix = f" ({', '.join(flags)})" if flags else ""
    if item.kind == "bool":
        return f"{tri_char(eff)}/{tri_char(req)}{suffix}"
    return f"{tri_char(eff)}/{tri_char(req)}{suffix}"


def option_state_word(item: Feature, ev: EvalResult, lang: str) -> str:
    eff = ev.effective.get(item.key, item.default)
    req = ev.requested.get(item.key, item.default)
    suffix: List[str] = []
    if not ev.visible.get(item.key, True):
        suffix.append(tr(lang, "hidden"))
    if ev.min_required.get(item.key, TRI_N) > TRI_N:
        suffix.append(tr(lang, "forced"))
    if ev.max_allowed.get(item.key, TRI_Y) <= TRI_N:
        suffix.append(tr(lang, "locked"))

    state_word = tri_word(eff)
    if lang == "zh":
        state_word = {"enabled": "启用", "module": "模块", "disabled": "禁用"}[state_word]

    base = f"{tri_char(eff)}/{tri_char(req)} {state_word}"
    if suffix:
        base += f" ({', '.join(suffix)})"
    return base


def render_option_details(item: Feature, ev: EvalResult, width: int = 76) -> List[str]:
    req = ev.requested.get(item.key, item.default)
    eff = ev.effective.get(item.key, item.default)
    floor = ev.min_required.get(item.key, TRI_N)
    ceil = ev.max_allowed.get(item.key, TRI_Y)
    dep = ev.dep_level.get(item.key, TRI_N)
    lines = [
        f"Key       : {item.key}",
        f"Title     : {item.title}",
        f"Kind      : {item.kind}",
        f"State     : {tri_char(eff)} ({tri_word(eff)}) / requested {tri_char(req)}",
        f"Visible   : {'yes' if ev.visible.get(item.key, True) else 'no'}",
        f"Depends   : {item.depends_on or '<none>'}",
        f"Dep level : {tri_char(dep)}",
        f"Floor/Ceil : {tri_char(floor)} / {tri_char(ceil)}",
        f"Selects   : {', '.join(item.selects) if item.selects else '<none>'}",
        f"Implies   : {', '.join(item.implies) if item.implies else '<none>'}",
    ]
    if item.description:
        lines.append("Description:")
        lines.extend(textwrap.wrap(item.description, width=width, subsequent_indent="  "))
    return lines


def render_option_details_localized(item: Feature, ev: EvalResult, lang: str, width: int = 76) -> List[str]:
    req = ev.requested.get(item.key, item.default)
    eff = ev.effective.get(item.key, item.default)
    floor = ev.min_required.get(item.key, TRI_N)
    ceil = ev.max_allowed.get(item.key, TRI_Y)
    dep = ev.dep_level.get(item.key, TRI_N)
    state_map = {"enabled": "启用", "module": "模块", "disabled": "禁用"} if lang == "zh" else None
    lines = [
        f"{tr(lang, 'current_option')}: {item.title}",
        f"{('键名' if lang == 'zh' else 'Key')}      : {item.key}",
        f"{('类型' if lang == 'zh' else 'Kind')}     : {item.kind}",
        f"{tr(lang, 'requested')} : {tri_char(req)} ({tri_word(req) if lang == 'en' else state_map[tri_word(req)]})",
        f"{tr(lang, 'effective')} : {tri_char(eff)} ({tri_word(eff) if lang == 'en' else state_map[tri_word(eff)]})",
        f"{('可见' if lang == 'zh' else 'Visible')}  : {'yes' if ev.visible.get(item.key, True) else 'no'}",
        f"{('依赖' if lang == 'zh' else 'Depends')}  : {item.depends_on or '<none>'}",
        f"{('依赖级别' if lang == 'zh' else 'Dep lvl')}  : {tri_char(dep)}",
        f"{('下限/上限' if lang == 'zh' else 'Floor/Ceiling')} : {tri_char(floor)} / {tri_char(ceil)}",
        f"{('选择' if lang == 'zh' else 'Selects')}  : {', '.join(item.selects) if item.selects else '<none>'}",
        f"{('蕴含' if lang == 'zh' else 'Implies')}  : {', '.join(item.implies) if item.implies else '<none>'}",
    ]
    if item.description:
        lines.append("Description:")
        lines.extend(textwrap.wrap(item.description, width=width, subsequent_indent="  "))
    return lines


def legacy_menu(features: Sequence[Feature], values: Dict[str, int], lang: str) -> bool:
    return main_loop(features, values)


def clamp_requested(item: Feature, raw: object) -> int:
    return normalize_tri(raw, item.default, item.kind)


def set_requested(values: Dict[str, int], item: Feature, raw: object) -> None:
    values[item.key] = clamp_requested(item, raw)


def cycle_requested(values: Dict[str, int], item: Feature, step: int = 1) -> None:
    current = values.get(item.key, item.default)
    if item.kind == "bool":
        values[item.key] = TRI_N if current > TRI_N else TRI_Y
        return

    allowed = [TRI_N, TRI_M, TRI_Y]
    if current not in allowed:
        current = item.default
    idx = allowed.index(current)
    values[item.key] = allowed[(idx + step) % len(allowed)]


def write_outputs(features: Sequence[Feature], ev: EvalResult) -> None:
    MENUCONFIG_DIR.mkdir(parents=True, exist_ok=True)

    output_values: Dict[str, object] = {}
    for item in features:
        value = ev.effective[item.key]
        if item.kind == "bool":
            output_values[item.key] = value >= TRI_Y
        else:
            output_values[item.key] = tri_char(value)

    CONFIG_JSON_PATH.write_text(
        json.dumps(output_values, ensure_ascii=True, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )

    lines = [
        "# Auto-generated by scripts/menuconfig.py",
        "# Do not edit manually unless you know what you are doing.",
        'set(XIAOBAIOS_MENUCONFIG_LOADED ON CACHE BOOL "XiaoBaiOS menuconfig loaded" FORCE)',
    ]
    for item in features:
        value = ev.effective[item.key]
        title = remap_text(item.title)
        if item.kind == "bool":
            lines.append(f'set({item.key} {"ON" if value >= TRI_Y else "OFF"} CACHE BOOL "{title}" FORCE)')
        else:
            lines.append(f'set({item.key} "{tri_char(value).upper()}" CACHE STRING "{title}" FORCE)')
            lines.append(
                f'set({item.key}_IS_ENABLED {"ON" if value > TRI_N else "OFF"} CACHE BOOL "{title} enabled" FORCE)'
            )

    CONFIG_CMAKE_PATH.write_text("\n".join(lines) + "\n", encoding="utf-8")


def print_main_menu(groups: Sequence[Tuple[str, List[Feature]]], features: Sequence[Feature], ev: EvalResult) -> None:
    enabled = sum(1 for item in features if ev.effective.get(item.key, item.default) > TRI_N)
    print()
    print("========== XiaoBaiOS menuconfig ==========")
    print(f"Enabled features: {enabled}/{len(features)}")
    print("Choose a group number, 's' to save, or 'q' to quit.")
    for idx, (group_name, items) in enumerate(groups, start=1):
        group_enabled = sum(1 for item in items if ev.effective.get(item.key, item.default) > TRI_N)
        print(f"{idx}) {group_name} ({group_enabled}/{len(items)} enabled)")
    print("s) Save and exit")
    print("q) Quit without saving")


def print_group_menu(group_name: str, items: Sequence[Feature], ev: EvalResult) -> None:
    print()
    print(f"== {group_name} ==")
    for idx, item in enumerate(items, start=1):
        print(f"{idx:2d}) {item.title} [{option_state(item, ev)}]")
        if item.description:
            wrapped = textwrap.wrap(item.description, width=72, subsequent_indent="     ")
            for line in wrapped:
                print(f"     {line}")
    print("Commands:")
    print("  <n>     cycle option n")
    print("  y <n>   set option n to y")
    print("  m <n>   set tristate option n to m")
    print("  n <n>   set option n to n")
    print("  d <n>   restore default for option n")
    print("  i <n>   show detailed info")
    print("  b       back to main menu")
    print("  s       save and exit")
    print("  q       quit without saving")


def curses_menu(features: Sequence[Feature], values: Dict[str, int], lang: str) -> bool:
    if curses is None:
        return False

    groups = grouped_features(features)
    group_index = 0
    option_index = 0
    focus = "groups"
    detail_scroll = 0

    def _group_label(name: str) -> str:
        return translate_group_name(name, lang)

    def _safe_addstr(win, y: int, x: int, text: str, attr: int = 0) -> None:
        h, w = win.getmaxyx()
        if y < 0 or y >= h or x >= w:
            return
        clipped = truncate_text(text, max(0, w - x - 1))
        try:
            win.addstr(y, x, clipped, attr)
        except curses.error:
            pass

    def _draw(stdscr) -> None:
        nonlocal detail_scroll
        stdscr.erase()
        h, w = stdscr.getmaxyx()
        ev = evaluate(features, values)
        current_group = groups[group_index]
        current_items = current_group[1]
        current_item = current_items[min(option_index, len(current_items) - 1)]

        _safe_addstr(stdscr, 0, 0, f" {tr(lang, 'title')} ", curses.A_REVERSE | curses.A_BOLD)
        lang_tag = f" {tr(lang, 'lang')}: {lang.upper()} "
        _safe_addstr(stdscr, 0, max(1, w - display_width(lang_tag) - 1), lang_tag, curses.A_REVERSE)

        left_w = max(24, min(36, w // 4))
        mid_w = max(30, min(48, (w - left_w) // 2))
        right_x = left_w + mid_w + 1
        divider1 = left_w
        divider2 = left_w + mid_w

        for y in range(1, h - 2):
            if divider1 < w:
                try:
                    stdscr.addch(y, divider1, curses.ACS_VLINE)
                except curses.error:
                    pass
            if divider2 < w:
                try:
                    stdscr.addch(y, divider2, curses.ACS_VLINE)
                except curses.error:
                    pass

        _safe_addstr(stdscr, 1, 1, tr(lang, 'group_pane'), curses.A_BOLD | curses.A_UNDERLINE)
        _safe_addstr(stdscr, 1, divider1 + 2, tr(lang, 'option_pane'), curses.A_BOLD | curses.A_UNDERLINE)
        _safe_addstr(stdscr, 1, divider2 + 2, tr(lang, 'detail_pane'), curses.A_BOLD | curses.A_UNDERLINE)

        group_rows = max(1, h - 6)
        option_rows = max(1, h - 8)
        detail_rows = max(1, h - 8)

        group_start = max(0, group_index - group_rows // 2)
        if group_start + group_rows > len(groups):
            group_start = max(0, len(groups) - group_rows)

        for row, idx in enumerate(range(group_start, min(len(groups), group_start + group_rows)), start=2):
            name, items = groups[idx]
            enabled, total = group_summary(items, ev)
            label = f" {idx + 1:2d}. {_group_label(name)} [{enabled}/{total}]"
            attr = curses.A_BOLD if idx == group_index and focus == "groups" else 0
            if idx == group_index:
                attr |= curses.A_REVERSE
            _safe_addstr(stdscr, row, 1, pad_text(label, left_w - 2), attr)

        _safe_addstr(stdscr, h - 3, 1, truncate_text(f"{tr(lang, 'current_group')}: {_group_label(current_group[0])}", w - 2), curses.A_DIM)

        option_start = max(0, option_index - option_rows // 2)
        if option_start + option_rows > len(current_items):
            option_start = max(0, len(current_items) - option_rows)

        for row, idx in enumerate(range(option_start, min(len(current_items), option_start + option_rows)), start=2):
            item = current_items[idx]
            state = option_state_word(item, ev, lang)
            label = f" {idx + 1:2d}. {item.title} [{state}]"
            attr = curses.A_BOLD if idx == option_index and focus == "options" else 0
            if idx == option_index:
                attr |= curses.A_REVERSE
            _safe_addstr(stdscr, row, divider1 + 2, pad_text(label, mid_w - 2), attr)

        detail_lines = render_option_details_localized(current_item, ev, lang, width=max(20, w - right_x - 3))
        max_scroll = max(0, len(detail_lines) - detail_rows)
        detail_scroll = max(0, min(detail_scroll, max_scroll))
        for row, line in enumerate(detail_lines[detail_scroll:detail_scroll + detail_rows], start=2):
            _safe_addstr(stdscr, row, divider2 + 2, truncate_text(line, max(0, w - divider2 - 3)))

        status_text = f"{tr(lang, 'status')}: {tr(lang, 'focus_groups') if focus == 'groups' else tr(lang, 'focus_options')}"
        _safe_addstr(stdscr, h - 2, 1, truncate_text(status_text, w - 2), curses.A_BOLD)
        _safe_addstr(stdscr, h - 1, 1, truncate_text(tr(lang, 'footer'), w - 2), curses.A_DIM)
        stdscr.refresh()

    def _prompt(stdscr, prompt: str) -> str:
        h, w = stdscr.getmaxyx()
        curses.echo()
        try:
            stdscr.move(h - 2, 1)
            stdscr.clrtoeol()
            _safe_addstr(stdscr, h - 2, 1, prompt, curses.A_BOLD)
            stdscr.refresh()
            raw = stdscr.getstr(h - 2, min(w - 2, 1 + display_width(prompt)), max(1, w - 2 - display_width(prompt)))
            return raw.decode("utf-8", errors="ignore").strip()
        finally:
            curses.noecho()

    def _apply(item: Feature, action: str) -> None:
        if action == "d":
            values[item.key] = item.default
        elif action == "y":
            values[item.key] = TRI_Y
        elif action == "m":
            values[item.key] = TRI_M if item.kind == "tristate" else TRI_Y
        elif action == "n":
            values[item.key] = TRI_N

    def _loop(stdscr) -> bool:
        nonlocal group_index, option_index, focus, detail_scroll
        try:
            curses.curs_set(0)
        except Exception:
            pass
        stdscr.keypad(True)
        if curses.has_colors():
            curses.start_color()

        while True:
            ev = evaluate(features, values)
            current_group = groups[group_index]
            current_items = current_group[1]
            option_index = min(option_index, max(0, len(current_items) - 1))
            current_item = current_items[option_index]
            detail_lines = render_option_details_localized(current_item, ev, lang)
            detail_scroll = max(0, min(detail_scroll, max(0, len(detail_lines) - max(1, stdscr.getmaxyx()[0] - 8))))
            _draw(stdscr)
            ch = stdscr.getch()

            if ch in {ord("q"), ord("Q")}: return False
            if ch in {ord("s"), ord("S")}: return True
            if ch in {curses.KEY_LEFT, curses.KEY_BTAB}: focus = "groups"; continue
            if ch in {curses.KEY_RIGHT, ord("\t")}: focus = "options"; continue

            if ch in {curses.KEY_UP, ord("k")}: 
                if focus == "groups":
                    group_index = max(0, group_index - 1)
                    option_index = min(option_index, len(groups[group_index][1]) - 1)
                else:
                    option_index = max(0, option_index - 1)
                detail_scroll = 0
                continue

            if ch in {curses.KEY_DOWN, ord("j")}: 
                if focus == "groups":
                    group_index = min(len(groups) - 1, group_index + 1)
                    option_index = min(option_index, len(groups[group_index][1]) - 1)
                else:
                    option_index = min(len(current_items) - 1, option_index + 1)
                detail_scroll = 0
                continue

            if ch in {curses.KEY_NPAGE}:
                if focus == "options":
                    detail_scroll += 3
                else:
                    group_index = min(len(groups) - 1, group_index + 1)
                    option_index = min(option_index, len(groups[group_index][1]) - 1)
                continue

            if ch in {curses.KEY_PPAGE}:
                if focus == "options":
                    detail_scroll = max(0, detail_scroll - 3)
                else:
                    group_index = max(0, group_index - 1)
                    option_index = min(option_index, len(groups[group_index][1]) - 1)
                continue

            if ch in {curses.KEY_ENTER, 10, 13}:
                if focus == "groups":
                    focus = "options"
                else:
                    cycle_requested(values, current_item)
                detail_scroll = 0
                continue

            if ch == ord(" "):
                if focus == "options":
                    cycle_requested(values, current_item)
                detail_scroll = 0
                continue

            if ch in {ord("y"), ord("Y"), ord("m"), ord("M"), ord("n"), ord("N"), ord("d"), ord("D")}: 
                _apply(current_item, chr(ch).lower())
                detail_scroll = 0
                continue

            if ch in {ord("i"), ord("I")}: 
                while True:
                    _draw(stdscr)
                    _safe_addstr(stdscr, stdscr.getmaxyx()[0] - 2, 1, truncate_text(tr(lang, 'help') + " - Enter/ESC 返回，↑↓/PgUp/PgDn 滚动详情", stdscr.getmaxyx()[1] - 2), curses.A_BOLD)
                    sub = stdscr.getch()
                    if sub in {27, ord("q"), ord("Q"), curses.KEY_ENTER, 10, 13}:
                        break
                    if sub in {curses.KEY_UP, ord("k")}: detail_scroll = max(0, detail_scroll - 1)
                    elif sub in {curses.KEY_DOWN, ord("j")}: detail_scroll += 1
                    elif sub == curses.KEY_NPAGE: detail_scroll += 5
                    elif sub == curses.KEY_PPAGE: detail_scroll = max(0, detail_scroll - 5)
                continue

            if ch in {ord("/"), ord("f"), ord("F")}: 
                query = _prompt(stdscr, "/ ")
                if not query:
                    continue
                query_l = query.lower()
                found = False
                for gi, (gname, gitems) in enumerate(groups):
                    if query_l in gname.lower() or query_l in _group_label(gname).lower():
                        group_index = gi
                        option_index = 0
                        focus = "options"
                        detail_scroll = 0
                        found = True
                        break
                    for oi, item in enumerate(gitems):
                        if query_l in item.title.lower() or query_l in item.key.lower() or query_l in item.description.lower():
                            group_index = gi
                            option_index = oi
                            focus = "options"
                            detail_scroll = 0
                            found = True
                            break
                    if found:
                        break
                continue

            if ord("1") <= ch <= ord("9"):
                idx = ch - ord("1")
                if focus == "groups" and idx < len(groups):
                    group_index = idx
                    option_index = 0
                    focus = "options"
                    detail_scroll = 0
                elif focus == "options" and idx < len(current_items):
                    option_index = idx
                    detail_scroll = 0

    return curses.wrapper(_loop)


def main_loop(features: Sequence[Feature], values: Dict[str, int]) -> bool:
    groups = grouped_features(features)
    index = {item.key: item for item in features}

    while True:
        ev = evaluate(features, values)
        print_main_menu(groups, features, ev)
        choice = input("Select> ").strip()
        lower = choice.lower()

        if lower in {"q", "quit"}:
            return False
        if lower in {"s", "save"}:
            return True

        if not choice.isdigit():
            print("Unknown selection")
            continue

        group_idx = int(choice) - 1
        if group_idx < 0 or group_idx >= len(groups):
            print("Invalid group number")
            continue

        group_name, items = groups[group_idx]
        while True:
            ev = evaluate(features, values)
            print_group_menu(group_name, items, ev)
            cmd = input(f"{group_name}> ").strip()
            if not cmd:
                continue
            cmd_lower = cmd.lower()
            if cmd_lower in {"b", "back"}:
                break
            if cmd_lower in {"q", "quit"}:
                return False
            if cmd_lower in {"s", "save"}:
                return True

            parts = cmd.split()
            if len(parts) == 1 and parts[0].isdigit():
                opt_idx = int(parts[0]) - 1
                if opt_idx < 0 or opt_idx >= len(items):
                    print("Invalid option number")
                    continue
                cycle_requested(values, items[opt_idx])
                continue

            if len(parts) == 2 and parts[1].isdigit():
                action = parts[0].lower()
                opt_idx = int(parts[1]) - 1
                if opt_idx < 0 or opt_idx >= len(items):
                    print("Invalid option number")
                    continue
                item = items[opt_idx]
                if action == "d":
                    values[item.key] = item.default
                elif action == "i":
                    for line in render_option_details(item, ev):
                        print(line)
                elif action == "y":
                    values[item.key] = TRI_Y
                elif action == "m":
                    values[item.key] = TRI_M if item.kind == "tristate" else TRI_Y
                elif action == "n":
                    values[item.key] = TRI_N
                else:
                    print("Unknown command")
                continue

            print("Unknown command")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Pure CLI XiaoBaiOS menuconfig")
    parser.add_argument("--defaults", action="store_true", help="ignore previous saved values and use defaults")
    parser.add_argument("--non-interactive", action="store_true", help="write outputs without opening the menu")
    parser.add_argument("--tui", action="store_true", help="use modern curses TUI when available")
    parser.add_argument("--plain", action="store_true", help="force legacy plain-text menu")
    parser.add_argument("--lang", choices=["auto", "en", "zh"], default="auto", help="select interface language")
    parser.add_argument(
        "--set",
        action="append",
        default=[],
        metavar="KEY=Y|M|N|TRUE|FALSE",
        help="override a value before saving (can be repeated)",
    )
    return parser.parse_args()


def apply_overrides(values: Dict[str, int], features: Sequence[Feature], overrides: Sequence[str]) -> None:
    index = {item.key: item for item in features}
    for entry in overrides:
        if "=" not in entry:
            raise RuntimeError(f"invalid --set entry: {entry!r}")
        key, raw = entry.split("=", 1)
        key = key.strip()
        if not key:
            raise RuntimeError(f"invalid --set entry: {entry!r}")
        item = index.get(key)
        if item is None:
            values[remap_namespace(key)] = normalize_tri(raw, TRI_N, "tristate")
        else:
            values[remap_namespace(key)] = normalize_tri(raw, item.default, item.kind)


def main() -> int:
    args = parse_args()
    features = load_features()
    lang = detect_language(args.lang)

    if args.tui and args.plain:
        raise RuntimeError("--tui and --plain cannot be used together")

    if args.defaults:
        values = {item.key: item.default for item in features}
    else:
        previous = load_previous_values()
        values = {item.key: normalize_tri(previous.get(item.key, item.default), item.default, item.kind) for item in features}

    apply_overrides(values, features, args.set)

    if args.non_interactive:
        ev = evaluate(features, values)
        write_outputs(features, ev)
        print(f"menuconfig: wrote {CONFIG_JSON_PATH}")
        print(f"menuconfig: wrote {CONFIG_CMAKE_PATH}")
        return 0

    if not sys.stdin.isatty():
        raise RuntimeError("menuconfig needs an interactive terminal, or use --non-interactive")

    should_save = False
    if args.plain:
        should_save = legacy_menu(features, values, lang)
    elif curses is not None:
        should_save = curses_menu(features, values, lang)
    else:
        should_save = legacy_menu(features, values, lang)
    if not should_save:
        print(tr(lang, "no_changes"))
        return 0

    ev = evaluate(features, values)
    write_outputs(features, ev)
    print(tr(lang, "saved"))
    print(f"menuconfig: wrote {CONFIG_JSON_PATH}")
    print(f"menuconfig: wrote {CONFIG_CMAKE_PATH}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except RuntimeError as exc:
        print(f"menuconfig error: {exc}", file=sys.stderr)
        raise SystemExit(1)
