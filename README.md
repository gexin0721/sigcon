# sigcon

An engineering-oriented graduation project scaffold for a traffic signal control system based on ESP32-S3 firmware, a Qt desktop supervisor, and HTTP communication.

## Planned Scope

- Firmware side: traffic phase control, device configuration, diagnostics, and HTTP service
- Desktop side: monitoring UI, device control, configuration management, and event review
- Hardware side: controller, lamp driving, power design, and basic schematic assets
- Engineering side: documentation, scripts, tests, logs, data directories, and CI placeholders

## Repository Layout

- `firmware/`: ESP32-S3 firmware project and components
- `desktop/`: Qt desktop application
- `shared/`: shared protocol and model definitions
- `hardware/`: schematic, PCB, BOM, and exported artifacts
- `docs/`: architecture, protocol, hardware, state machine, and thesis material
- `scripts/`: local automation scripts for build, run, test, and packaging
- `tests/`: integration and end-to-end test assets
- `runtime/`: local logs and runtime data, ignored by Git except placeholders

## Status

This repository is currently scaffolded for structure and documentation. Implementation can now be added module by module.
