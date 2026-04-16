# sigcon firmware

`firmware/` is the ESP32-S3 firmware workspace for the traffic signal control system.

## Current Status

- Old reusable modules are still kept in the repository for later selection and refactoring.
- `main/` now contains only a minimal bootstrap loop for the current project.

## Build

```bash
idf.py set-target esp32s3
idf.py build
idf.py flash
idf.py monitor
```

## Notes

- `main/` is reserved for the active traffic signal firmware entry.
- Existing `Hardware/`, `System/`, and `test/` content is kept for reuse and later cleanup.
- Traffic phase control, HTTP service, configuration storage, and diagnostics can be added incrementally from here.
