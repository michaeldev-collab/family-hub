# Hardware

Physical build assets for Family Hub (Waveshare ESP32-S3-Touch-LCD-7B wall panel).

```text
hardware/
├── reference/
│   ├── measurements/   # drawings, caliper notes
│   ├── photos/         # build / fit photos
│   └── wiring/         # switch, power, SD notes
├── cad/
│   ├── family-hub-7b-case.scad   ← source of truth
│   ├── family-hub-7b-case.stl
│   └── exports/                 # STEP, 3MF, sliced previews, etc.
└── docs/                        # human writeups for the case
```

## Case (current)

- Source: [`cad/family-hub-7b-case.scad`](./cad/family-hub-7b-case.scad)
- Mesh: [`cad/family-hub-7b-case.stl`](./cad/family-hub-7b-case.stl)
- Notes: [`docs/case-7b.md`](./docs/case-7b.md)

```bash
cd hardware/cad
openscad -o family-hub-7b-case.stl family-hub-7b-case.scad
```
