# Scenario schema

Scenario files are JSON documents with schema version `1`.

Required root fields:

| Field | Type | Unit/meaning |
|---|---|---|
| `schema_version` | integer | Schema identity; currently `1` |
| `scenario_id` | string | Stable scenario identifier |
| `simulation_version` | string | Simulation model version captured in output |
| `fixed_timestep_seconds` | number | Fixed simulation step duration in seconds |
| `rooms` | array | Room definitions |
| `plants` | array | Plant identity and location definitions |

Room fields:

- `id`: stable room identifier
- `width_m`: room width in metres
- `depth_m`: room depth in metres
- `cell_size_m`: spatial-cell edge length in metres
- `cells`: optional explicit spatial-cell input definitions. When present, each cell requires a physical environment and lighting schedule.

Explicit cell fields:

- `id`: stable cell identifier
- `center_x_m`, `center_y_m`: cell location in metres
- `environment.air_temperature_c`: air temperature in degrees Celsius
- `environment.relative_humidity_percent`: relative humidity from 0 to 100 percent
- `environment.atmospheric_pressure_kpa`: atmospheric pressure in kilopascals
- `environment.co2_umol_per_mol`: CO2 concentration in micromoles per mole
- `environment.leaf_temperature_c`: leaf temperature in degrees Celsius or `null`
- `lighting_schedule`: non-overlapping piecewise-constant segments
- `lighting_schedule.start_seconds` and `end_seconds`: seconds from the simulation-day boundary
- `lighting_schedule.ppfd_umol_per_m2_s`: PPFD in micromoles photons per square metre per second

Plant fields:

- `id`: stable plant identifier
- `room_id`: referenced room identifier
- `cultivar_id`: genetics reference identifier, not a biological model
- `x_m`, `y_m`, `z_m`: plant position in metres in the room coordinate system

The bootstrap rejects unlabeled alternatives such as `fixed_timestep_hours`, non-positive dimensions, duplicate identifiers, unknown room references, out-of-bounds plant locations, invalid RH, invalid pressure, invalid PPFD, and overlapping lighting segments. Unconfigured rooms remain supported for P0 compatibility, but their cell physics are explicitly unavailable rather than populated with guessed defaults.

The machine-readable schema is [`scenario.schema.json`](</home/jd/CannaVille-V2/Docs/scenario.schema.json>). The executable smoke scenario is [`smoke_test.json`](</home/jd/CannaVille-V2/Scenarios/smoke_test.json>).
