# Model registry

The registry is the review record for future scientific and gameplay models. It is deliberately separate from runtime configuration.

Each entry must identify the model, version, implementation status, approximation class, sources, variables, constants, valid ranges, calibration status, limitations, gameplay parameters, and regression scenarios.

Runtime code should reference a registry model identifier rather than embedding an undocumented equation or constant. A registry entry marked `placeholder` is an interface declaration only and must not be presented as scientific behavior.

The machine-readable contract is [`model_registry.schema.json`](</home/jd/CannaVille-V2/Docs/model_registry.schema.json>). No biological model entries are included in this bootstrap.
