# Family Hub — Waveshare 7B wall case

## Mounting

| Joint | Method |
| --- | --- |
| Panel → case | Waveshare peelable adhesive sticker → flat pad inside the back |
| Case → wall | **Two** Velcro Command strips (recesses on outer back) |

No M3 screws / standoffs. No raised seat lip under the glass — the **outer front rim** frames the panel edge.

## Access

- **Left:** large bay for USB-C / microSD
- **Top:** large bay for Boot / Reset / SD
- **Right:** MX switch bay (PCB up, switch at bottom)

Corner radius: **4 mm**. Electronics cavity: **22 mm**.

## Source of truth

| File | Role |
| --- | --- |
| [`../cad/family-hub-7b-case.scad`](../cad/family-hub-7b-case.scad) | Parametric source |
| [`../cad/family-hub-7b-case.stl`](../cad/family-hub-7b-case.stl) | Print mesh |
| [`../cad/exports/`](../cad/exports/) | Extra exports (STEP, 3MF, …) |

```bash
cd hardware/cad
openscad -o family-hub-7b-case.stl family-hub-7b-case.scad
```

## Tweak after dry-fit

- `sticker_pad_w` / `sticker_pad_h` — match peel sticker
- `cmd_w` / `cmd_h` / `cmd_gap` — match Velcro Command strips
- `left_access_*` / `top_access_*` — access bays
- `sw_from_bottom` / `sw_z` — MX bay

See also [`../reference/measurements/`](../reference/measurements/).
