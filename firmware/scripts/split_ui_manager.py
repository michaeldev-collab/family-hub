#!/usr/bin/env python3
"""Split ui_manager.cpp by FAMILY_HUB_APP_* guards into household/child files.

Child extract path is archived (2026-07-26) — live Family Hub is household-only.
Do not regenerate into a live src/child/ without restoring the child-panel archive.
"""

from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "archive" / "ui_manager.cpp"
HOUSEHOLD_OUT = ROOT / "src" / "household" / "household_ui.cpp"
CHILD_OUT = ROOT / "src" / "child" / "child_ui.cpp"

# Implemented in ui_manager_core.cpp — omit from household/child extracts.
SKIP_FUNCTIONS = {
    "begin",
    "showWriteResult",
    "consumeRenderRequest",
    "requestSync",
    "consumeSyncRequest",
    "badgeLabel",
    "renderStatusBar",
    "resetLvglPointers",
}

HOUSEHOLD_DEFS = {"FAMILY_HUB_APP_HOUSEHOLD": True, "WAVESHARE_7B": True}
CHILD_DEFS = {"FAMILY_HUB_APP_CHILD": True, "WAVESHARE_7B": True}


def parse_directive(line: str):
    m = re.match(r"^\s*#\s*(if|ifdef|ifndef|elif|else|endif)\b(.*)$", line)
    if not m:
        return None
    kind, rest = m.group(1), m.group(2).strip()
    if kind == "else":
        return ("else", None)
    if kind == "endif":
        return ("endif", None)
    if kind == "elif":
        return ("elif", rest)
    return (kind, rest)


def eval_cond(expr: str, defs: dict[str, bool]) -> bool:
    expr = expr.strip()
    if not expr:
        return False
    tokens = re.findall(
        r"defined\s*\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)"
        r"|!defined\s*\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)"
        r"|([A-Za-z_][A-Za-z0-9_]*)"
        r"|(\|\|)|(&&)|(!)",
        expr,
    )
    # Simple recursive descent for || and &&
    pos = 0

    def peek():
        return tokens[pos] if pos < len(tokens) else None

    def consume():
        nonlocal pos
        t = peek()
        pos += 1
        return t

    def primary():
        t = consume()
        if not t:
            return False
        if t[0]:
            return defs.get(t[0], False)
        if t[1]:
            return not defs.get(t[1], False)
        if t[2]:
            if t[2] == "defined":
                return False
            return defs.get(t[2], False)
        if t[4] == "!":
            return not primary()
        return False

    def unary():
        return primary()

    def and_expr():
        val = unary()
        while peek() and peek()[3] == "&&":
            consume()
            val = val and unary()
        return val

    def or_expr():
        val = and_expr()
        while peek() and peek()[4] == "||":
            consume()
            val = val or and_expr()
        return val

    # Re-tokenize simply
    parts = re.split(r"\s+(\|\||&&)\s+", expr)
    if len(parts) == 1:
        return eval_atom(expr.strip(), defs)

    val = eval_atom(parts[0].strip(), defs)
    i = 1
    while i < len(parts):
        op = parts[i]
        rhs = eval_atom(parts[i + 1].strip(), defs)
        if op == "||":
            val = val or rhs
        else:
            val = val and rhs
        i += 2
    return val


def eval_atom(atom: str, defs: dict[str, bool]) -> bool:
    atom = atom.strip()
    if atom.startswith("(") and atom.endswith(")"):
        return eval_cond(atom[1:-1], defs)
    m = re.match(r"!defined\s*\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)", atom)
    if m:
        return not defs.get(m.group(1), False)
    m = re.match(r"defined\s*\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)", atom)
    if m:
        return defs.get(m.group(1), False)
    if atom.startswith("!"):
        return not eval_atom(atom[1:].strip(), defs)
    return defs.get(atom, False)


def extract(defs: dict[str, bool]) -> str:
    lines = SRC.read_text().splitlines()
    out: list[str] = []
    stack: list[tuple[bool, bool]] = []  # (parent_active, branch_active)
    active = True
    in_skip_fn = False
    brace_depth = 0

    for n, line in enumerate(lines, 1):
        stripped = line.strip()
        if in_skip_fn:
            brace_depth += line.count("{") - line.count("}")
            if brace_depth <= 0 and "}" in line:
                in_skip_fn = False
            continue
        for fn in SKIP_FUNCTIONS:
            if f"UiManager::{fn}" in line:
                in_skip_fn = True
                brace_depth = line.count("{") - line.count("}")
                break
        if in_skip_fn:
            continue

        directive = parse_directive(line)
        if directive:
            kind, expr = directive
            if kind == "if":
                parent = active
                branch = parent and eval_cond(expr or "", defs)
                stack.append((parent, branch))
                active = branch
                continue
            if kind == "ifdef":
                parent = active
                sym = (expr or "").split()[0]
                branch = parent and defs.get(sym, False)
                stack.append((parent, branch))
                active = branch
                continue
            if kind == "ifndef":
                parent = active
                sym = (expr or "").split()[0]
                branch = parent and not defs.get(sym, False)
                stack.append((parent, branch))
                active = branch
                continue
            if kind == "elif":
                parent, prev = stack[-1]
                if prev:
                    stack[-1] = (parent, False)
                    active = False
                else:
                    branch = parent and eval_cond(expr or "", defs)
                    stack[-1] = (parent, branch)
                    active = branch
                continue
            if kind == "else":
                parent, prev = stack[-1]
                branch = parent and not prev
                stack[-1] = (parent, branch)
                active = branch
                continue
            if kind == "endif":
                stack.pop()
                active = stack[-1][1] if stack else True
                continue

        if not active:
            continue
        if stripped.startswith("#"):
            continue
        out.append(line)

    return "\n".join(out).rstrip() + "\n"


def header_for(mode: str) -> str:
    if mode == "child":
        extra = '#include "child_focus_state.h"\n'
    else:
        extra = ""
    return (
        '#include "ui_manager.h"\n\n'
        "#ifdef WAVESHARE_7B\n"
        "#include <lvgl.h>\n"
        '#include "display.h"\n'
        '#include "panel_config.h"\n'
        f"{extra}"
        "#endif\n\n"
    )


def main():
    household_body = extract(HOUSEHOLD_DEFS)
    child_body = extract(CHILD_DEFS)
    HOUSEHOLD_OUT.write_text(header_for("household") + household_body)
    CHILD_OUT.write_text(header_for("child") + child_body)
    print(f"Wrote {HOUSEHOLD_OUT} ({HOUSEHOLD_OUT.stat().st_size} bytes)")
    print(f"Wrote {CHILD_OUT} ({CHILD_OUT.stat().st_size} bytes)")


if __name__ == "__main__":
    main()
