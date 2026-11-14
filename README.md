# Aether

A layered synth plugin (VST3 + AU) with 1,169 presets, ported from the Aether browser synth.

- `src/` — the synth engine (no framework dependencies), patch loading and output effects
- `plugin/` — the JUCE plugin: processor and window
- `resources/presets.json.gz` — the preset library
- `tools/export.js` — regenerates the library and the port-test references from the browser synth
- `tests/` — engine port test (sample-exact against the browser engine) and a headless plugin test

Builds run on GitHub Actions on every push to `main`; the latest Mac and Windows downloads are attached to the `latest` release.
