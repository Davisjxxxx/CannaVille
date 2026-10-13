# Model registry

The registry is the review record for future scientific and gameplay models. It is deliberately separate from runtime configuration.

Each entry must identify the model, version, purpose, equation or algorithm, inputs, outputs, units, source title, authors, publication or organization, DOI or stable identifier, valid domain, assumptions, approximation status, calibration status, implementation location, validation tests, and notes.

Runtime code should reference a registry model identifier rather than embedding an undocumented equation or constant. A registry entry marked `placeholder` is an interface declaration only and must not be presented as scientific behavior.

The machine-readable entry contract is [`model_registry.schema.json`](</home/jd/CannaVille-V2/Docs/model_registry.schema.json>). A registry document contract is [`model_registry_document.schema.json`](</home/jd/CannaVille-V2/Docs/model_registry_document.schema.json>). P1A entries are in [`model_registry_p1a.json`](</home/jd/CannaVille-V2/Scenarios/model_registry_p1a.json>); P1B.5 leaf-energy entries are in [`model_registry_p1b5.json`](</home/jd/CannaVille-V2/Scenarios/model_registry_p1b5.json>).
P2 consequence entries are in [`model_registry_p2.json`](</home/jd/CannaVille-V2/Scenarios/model_registry_p2.json>). They classify relative opportunity, stage progression, exposure/recovery/damage, and presentation projection as bounded gameplay models rather than calibrated Cannabis physiology.
