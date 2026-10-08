# Unit conventions

The simulation uses SI-oriented units internally. Every biological or environmental quantity must have a unit-bearing field name or a named C++ quantity wrapper. Bare biological numeric constants are prohibited at model interfaces.

| Quantity | Internal convention | Example field suffix/type |
|---|---|---|
| Time | seconds | `_seconds`, `units::Seconds` |
| Distance | metres | `_m`, `units::Meters` |
| Temperature | degrees Celsius | `_c`, `units::Celsius` |
| Relative humidity | fraction from 0 to 1 | `_fraction`, `units::RelativeHumidityFraction` |
| CO2 | micromoles per mole | `_umol_per_mol`, `units::CO2MicromolesPerMole` |
| Airflow | metres per second | `_m_per_s`, `units::AirflowMetersPerSecond` |
| PPFD | micromoles per square metre per second | `_umol_per_m2_s` |
| DLI | moles per square metre per day | `_mol_per_m2_day` |
| Photoperiod | hours | `_hours` |
| pH | dimensionless pH scale | `ph`, `units::PH` |
| EC | millisiemens per centimetre | `_ms_per_cm` |
| Substrate moisture | fraction | `_fraction` |
| Mass | grams | `_g`, `units::MassGrams` |

## Rules

1. A field name must communicate its unit, or its type must be a named unit wrapper.
2. Conversion belongs at explicit boundaries, not inside unrelated model code.
3. Display units are presentation concerns. They must not change internal state units.
4. Constants require metadata in the model registry: units, source, calibration status, and valid range.
5. A value with unknown units is invalid input, not an implicit assumption.

The bootstrap uses zero-valued placeholder state where no model has been approved. Zero is an inert initialization value, not a horticultural claim.
