# P1B.5R — Leaf Energy Balance Research and Design

**Status:** proposed implementation contract; research/design only
**Baseline:** `v0.6.0-p1b41-hydraulic-limitation` / `98c5780b188ec9e0f6ea63a985df1523127cad0b`
**Scope:** leaf-temperature feedback for controlled-environment simulation
**Production code changed in this sprint:** none

This document is the research closure for P1B.5R. It deliberately does not implement
the solver, add biological response models, modify the accepted P1B.4.1 equations, or
change the model registry. The identifiers and interfaces below are proposed inputs to
the next implementation sprint and are not yet authoritative model-registry entries.

## 1. Executive recommendation

Implement one bounded, deterministic, steady-state leaf-energy solve per plant and
fixed simulation segment. The solver should find the leaf temperature `T_leaf` for
which the net energy residual is zero:

```text
F(T_leaf) = R_sw_abs(T_leaf-independent)
          + R_lw_net(T_leaf)
          - H(T_leaf)
          - LE(T_leaf)
          = 0
```

The first implementation should:

1. require explicit incident shortwave energy in `W m^-2`; it must not convert PPFD
   to energy with a hidden default;
2. use an explicit radiative-surroundings temperature rather than silently treating
   air temperature as sky/wall temperature;
3. use one scalar broadband shortwave absorptance and an explicitly configured
   longwave emissivity;
4. reuse the existing boundary-layer and P1B.4.1 transpiration pipeline, extending
   the boundary-layer result to expose the heat-transfer quantity needed by the
   energy balance;
5. evaluate gas exchange, boundary-layer conductance, and transpiration at each
   trial `T_leaf`, while keeping the beginning-of-segment hydraulic state fixed;
6. use a deterministic bracketed bisection solver with explicit no-bracket,
   non-finite, unavailable-input, unsupported-boundary, and iteration-limit states;
7. preserve external measured leaf temperature through an explicit mode rather than
   silently replacing it with a computed value.

The selected first model is therefore **steady-state, explicit-radiation,
single-leaf, mixed-convection, scalar-root energy balance**. Dynamic thermal storage,
canopy radiation transport, CFD, and facility heat balance are not part of P1B.5.

## 2. Product justification and stopping rule

Leaf temperature closes a valuable causal loop that is currently missing:

```text
radiation -> leaf temperature -> leaf VPD -> stomata
          -> transpiration -> latent cooling -> leaf temperature
```

That loop is directly useful to the game because it can make stronger light, weak
airflow, limited transpiration, and explicit radiative surroundings produce different
observable environmental and diagnostic outcomes without adding arbitrary stress
multipliers. It is also the narrowest addition that prevents the existing leaf-VPD
input from remaining a permanently external number.

P1B.5 is the last planned foundational physical-physiology subsystem. After its
implementation and one closure review, the roadmap pivots to **P2 — Plant Growth and
Gameplay Consequence Layer**. P2 may integrate time, stage, visible vigor, symptoms,
interventions, recovery, yield, and quality. P1B.5 must not grow into a crop-research
platform before that pivot.

## 3. Governing physics

### 3.1 Selected balance

The general leaf balance is:

```text
R_n - H - LE = S
```

where:

| Symbol | Meaning | Internal unit |
|---|---|---|
| `R_n` | net absorbed radiation | `W m^-2` |
| `H` | sensible heat flux, positive from leaf to air | `W m^-2` |
| `LE` | latent heat flux, positive for evaporation from leaf | `W m^-2` |
| `S` | physical heat storage | `W m^-2` |

The selected P1B.5 approximation is `S = 0`. This is a numerical/modeling assumption,
not a claim that real leaves have zero thermal inertia. Under a fixed controlled
environment, published leaf-energy models treat equilibrium as a useful approximation
that is generally reached within minutes. P1B.5 simulation segments already represent
piecewise-constant environmental forcing, but a short segment can still be physically
too short for equilibrium. The implementation must therefore expose segment duration
in diagnostics and label the result `steady_state_segment_estimate`; it must not call
the output a transient prediction.

The evidence for this choice is:

- Bonan describes leaf temperature as the result of energy absorbed by a leaf and
  dissipated as sensible and latent heat, with boundary-layer conductance controlled
  by wind and leaf size.
- Vialet-Chabrand and Lawson give a steady-state balance and a separate dynamic form
  containing physical storage, leaf density, heat capacity, and thickness.
- `tealeaves` solves the equilibrium root of the leaf energy budget and reports that
  equilibrium is generally reached within a few minutes.

Sources are listed in Section 4.

### 3.2 Sign convention and components

All component outputs use positive magnitudes for the direction named in the table:

```text
R_sw_abs  positive into the leaf
R_lw_net  positive into the leaf
H         positive out of the leaf to the air
LE        positive out of the leaf by evaporation
F         positive when the leaf has a net energy surplus
```

Thus:

```text
F(T_leaf) = R_sw_abs + R_lw_net - H - LE
```

Negative `LE` is a physically meaningful condensation case: condensation releases
latent heat to the leaf. It must be distinguishable from an unavailable transpiration
calculation. A negative energy flux does not automatically authorize negative plant
water withdrawal; the existing water-ledger contract must continue to distinguish
water withdrawal from any future condensation-return accounting.

### 3.3 Units

The P1B.5 design uses SI internally, consistent with the existing governance and
units documents:

| Quantity | Unit |
|---|---|
| temperature | `degC` for external scenario values; `K` for radiation equations |
| pressure | `kPa` at the environment boundary; `Pa` when used in ideal-gas equations |
| RH | `%`, validated in `[0, 100]` |
| PPFD | `umol photons m^-2 s^-1` |
| shortwave energy | `W m^-2` |
| vapor pressure and VPD | `kPa` |
| airflow | `m s^-1` |
| characteristic leaf dimension | `m` |
| conductance velocity form | `m s^-1` |
| conductance molar form | `mol m^-2 s^-1` |
| heat flux | `W m^-2` |
| transpiration | `mol H2O m^-2 s^-1` |
| time | `s` internally |

No numeric field may be added without its unit-bearing name or strong unit type.

## 4. Source and evidence table

| Source | Evidence used | Classification and boundary |
|---|---|---|
| Campbell & Norman, *An Introduction to Environmental Biophysics*, 2nd ed. (1998), [Google Books](https://books.google.com/books?id=v6UpE6lThCwC) | General leaf radiation, sensible/latent heat, boundary layer, and environmental biophysics foundations | `PEER_REVIEWED_GENERAL_PLANT` / authoritative technical reference; not Cannabis-specific |
| Bonan, “Leaf Temperature and Energy Fluxes,” in *Climate Change and Terrestrial Ecosystem Modeling* (2019), [doi:10.1017/9781107339217.011](https://doi.org/10.1017/9781107339217.011) | Net radiation, sensible and latent heat partitioning; leaf boundary conductance depends on wind and leaf size | `PEER_REVIEWED_GENERAL_PLANT`; canopy-scale extensions remain out of scope |
| Muir, “tealeaves: an R package for modelling leaf temperature using energy budgets,” *AoB PLANTS* 11 (2019), [doi:10.1093/aobpla/plz054](https://doi.org/10.1093/aobpla/plz054), [PMC](https://pmc.ncbi.nlm.nih.gov/articles/PMC6899345/) | Explicit equilibrium residual, root-solving approach, two-sided radiation/conductance examples, heat and latent flux relationships, SI-unit discipline | `PEER_REVIEWED_GENERAL_PLANT`; example defaults are not CannaVille constants |
| Vialet-Chabrand & Lawson, “Dynamic leaf energy balance: deriving stomatal conductance from thermal imaging in a dynamic environment,” *Journal of Experimental Botany* 70 (2019), [doi:10.1093/jxb/erz068](https://doi.org/10.1093/jxb/erz068), [PMC](https://pmc.ncbi.nlm.nih.gov/articles/PMC6506762/) | Steady-state versus storage balance, units, sensible heat, latent heat, and the role of physical storage in a dynamic model | `PEER_REVIEWED_GENERAL_PLANT`; dynamic storage deliberately deferred |
| FAO-56 Annex 3, “Background on physical parameters used in evapotranspiration computations,” [FAO](https://www.fao.org/4/X0490E/x0490e0k.htm) | Temperature-dependent latent heat of vaporization: `lambda = 2.501 - 0.002361*T_C` MJ kg^-1 over normal environmental ranges | `RECOGNIZED_HORTICULTURAL_ENGINEERING`; implementation must register its valid range and conversion |
| McCree, “Test of current definitions of photosynthetically active radiation against leaf photosynthesis data,” *Agricultural Meteorology* 9 (1972), [doi:10.1016/0002-1571(72)90045-3](https://doi.org/10.1016/0002-1571%2872%2990045-3) | PPFD is a photon-flux representation whose relationship to energy depends on the radiation spectrum | `PEER_REVIEWED_PRIMARY`; supports prohibition on hidden universal PPFD-to-energy conversion |
| Chandra et al., “Photosynthetic response of *Cannabis sativa* L. to variations in PPFD, temperature and CO2 conditions,” *Physiology and Molecular Biology of Plants* (2008), [PMC](https://pmc.ncbi.nlm.nih.gov/articles/PMC3550641/) | Cannabis gas-exchange responses vary with PPFD, temperature, CO2, and tested conditions; useful as later calibration evidence, not leaf-energy constants | `CANNABIS_SPECIFIC`, limited to the reported cultivar/conditions; no leaf-energy parameter is generalized |
| Arve et al., “Supplemental greenhouse lighting increased the water use efficiency, crop growth, and cutting production in *Cannabis sativa*,” *Frontiers in Plant Science* (2024), [doi:10.3389/fpls.2024.1371702](https://doi.org/10.3389/fpls.2024.1371702) | Controlled Cannabis leaf gas-exchange and leaf-temperature observations under stated chamber conditions; demonstrates that PPFD, DLI, transpiration, and leaf temperature are jointly measured | `CANNABIS_SPECIFIC`, cultivar and protocol bound; not a universal energy-balance calibration |

The source hierarchy is intentionally general-plant first. The Cannabis studies support
later validation and calibration questions but do not justify Cannabis-specific leaf
absorptance, emissivity, thermal inertia, airflow correlations, or survival limits.

## 5. Radiation architecture

### 5.1 PPFD is not radiant energy

`PPFD` counts photons in the photosynthetically active band. Leaf energy balance needs
radiant power. The conversion depends on the spectral power distribution and is not a
universal property of PPFD. McCree's work explicitly distinguishes energy flux,
incident quantum flux, absorbed quantum flux, and light-source spectral distributions.

Therefore:

```text
horticultural light state != rendering light state
PPFD != incident shortwave energy
```

The existing PPFD remains authoritative for photosynthetic/gas-exchange inputs. It is
never read from Unreal renderer brightness and is never silently converted to
`W m^-2`.

### 5.2 Selected shortwave representation

Select **Option A: explicit incident shortwave energy** for P1B.5:

```text
LightState.ppfd_umol_m2_s
LightState.incident_shortwave_w_m2 (optional but required for solved mode)
```

Both values are per environmental cell and can vary at exact light-event boundaries.
An explicit fixture profile may later provide a measured spectrum-specific relation,
but that is a separate model and must not override an explicitly supplied energy value.

If a scenario has PPFD but no incident shortwave energy and no explicitly configured
fixture conversion, solved leaf energy status is `missing_incident_shortwave`. No
default `W m^-2 per PPFD` coefficient exists.

### 5.3 Absorbed shortwave

The first implementation uses a scalar broadband leaf absorptance:

```text
R_sw_abs = alpha_sw * R_sw_incident
```

where `alpha_sw` is dimensionless and explicitly configured on the leaf-energy
profile. The initial model does not split PAR and NIR, trace spectra, or solve leaf
orientation/ray interception. The configuration must record whether the scalar is a
broadband fixture/leaf approximation.

The following are not production constants yet:

- broadband `alpha_sw`;
- spectral absorptance;
- orientation/view factor;
- canopy interception or self-shading.

Each is `CALIBRATION_REQUIRED` or `OUT_OF_SCOPE` as noted in Section 16. Any fixture
or synthetic test value must be labeled `SYNTHETIC_TEST_VALUE`.

### 5.4 Longwave radiation

Use an explicit radiative-surroundings temperature:

```text
R_lw_net = surface_factor * emissivity_lw * sigma
            * (T_surroundings_K^4 - T_leaf_K^4)
```

`R_lw_net` is positive when the surroundings radiate more energy to the leaf than the
leaf emits. The first model uses a configured `surface_factor` of one or two as an
explicit geometry choice; two is the normal two-sided leaf representation when the
scenario supports it. It is not a hidden assumption. The factor may not be used to
pretend that an entire canopy has been modeled.

The first model requires `radiative_surroundings_temperature_c` in solved mode. Air
temperature is not an automatic sky, wall, or fixture-temperature proxy. If the
scenario author deliberately wants an air-temperature proxy, it must be written into
the scenario as an explicit approximation with evidence class
`EMPIRICAL_APPROXIMATION` and a named model-registry entry in the future
implementation sprint.

`emissivity_lw` is a leaf-profile input. Stefan–Boltzmann's constant is a physical
constant. Neither should be tuned as a gameplay parameter.

## 6. Sensible heat model

Use the same boundary-layer state that supports transpiration. Do not create a second
wind/leaf-size model that can disagree with P1B.4.1.

The energy layer should consume a heat-transfer boundary conductance in velocity form:

```text
H = surface_factor * g_bh_m_s * rho_air_kg_m3
    * cp_air_J_kg_K * (T_leaf_C - T_air_C)
```

The sign is positive for a leaf warmer than the air. The accepted implementation
currently exposes a vapor boundary-layer conductance in `mol m^-2 s^-1` and an
explicit `UnsupportedFlowRegime` state. The P1B.5 implementation should extend the
boundary-layer API so one supported aerodynamic calculation provides both:

```text
g_bw  vapor conductance, existing authoritative quantity
g_bh  heat conductance, new energy-balance quantity
```

The recommended implementation route is the shared dimensionless-convection
calculation used by the existing boundary-layer model, with the appropriate heat
transport property (Nusselt/Prandtl form) and the existing water-vapor property
(Sherwood/Schmidt form). This preserves the existing Reynolds-domain guard and
avoids duplicating the airflow/leaf-dimension physics. A Lewis-relation shortcut may
be used only if the exact conductance convention and valid domain are registered;
it must not be inserted as an unexplained scalar.

The energy result is unavailable if the shared boundary-layer calculation cannot
return an authoritative heat conductance. Gas exchange convergence alone is not
sufficient to make `H`, `LE`, `T_leaf`, or water withdrawal valid.

The implementation must explicitly record the conversion path and units for:

- air density, `kg m^-3`;
- humid-air heat capacity, `J kg^-1 K^-1`;
- heat conductance, `m s^-1`;
- the relation between heat and vapor conductance.

## 7. Latent heat model

P1B.4.1 remains the sole source of transpiration flux. P1B.5 must not calculate a
competing stomatal or water-vapor flux.

For an available flux `E_mol`:

```text
LE_W_m2 = E_mol_H2O_m2_s * lambda_molar_J_mol
```

The proposed conversion is:

```text
lambda_mass_J_kg = 1,000,000 * (2.501 - 0.002361 * T_reference_C)
lambda_molar_J_mol = lambda_mass_J_kg * water_molar_mass_kg_mol
```

The first implementation should use leaf temperature as `T_reference_C`, because the
latent energy is being evaluated at the solved leaf surface. The reference temperature
choice must be registered and tested against the accepted flux units. The FAO-56
relationship is nearly constant over normal horticultural ranges, but its documented
valid range and approximation status must be carried into the registry.

When P1B.4.1 returns `condensation`, the negative flux may produce negative `LE` in
the energy residual. The water-ledger adapter must not turn that into a negative
withdrawal. Condensation return/storage is a separate future accounting decision.

When P1B.4.1 returns unavailable transpiration, `LE` is unavailable, not zero, even
when gas exchange is converged.

## 8. Circular coupling analysis

The loop is real and must be solved as one coupled leaf-temperature problem:

```text
trial T_leaf
  -> saturation vapor pressure at trial T_leaf
  -> leaf VPD from ambient actual vapor pressure
  -> existing FvCB/Medlyn gas-exchange solve
  -> stomatal conductance
  -> shared boundary-layer solve
  -> existing transpiration calculation
  -> latent heat conversion
  -> sensible heat and longwave exchange
  -> energy residual
  -> next trial T_leaf
```

For one segment, the following remain fixed while the scalar solve runs:

- the segment's air temperature, RH, pressure, CO2, airflow, PPFD, and explicit
  shortwave/radiative inputs;
- the beginning-of-segment substrate/DWC state;
- the hydraulic profile and the beginning-of-segment hydraulic state/beta;
- leaf profile parameters and effective transpiring area;
- simulation version, model versions, and seed (the physical solve itself adds no
  random draws).

The following are recomputed for each trial temperature:

- leaf saturation vapor pressure and leaf VPD;
- gas-exchange state that depends on leaf temperature/VPD;
- boundary-layer conductance if the heat/vapor property calculation depends on
  temperature;
- transpiration flux and latent energy;
- energy residual.

Root-zone inventory is not iterated simultaneously with leaf temperature. After the
root converges, the final authoritative transpiration result generates the existing
water request. Segment-level water event and light segmentation remains exact; the
energy solve is nested inside each fixed-forcing segment.

## 9. Numerical solver comparison

| Strategy | Advantages | Risks for CannaVille | Decision |
|---|---|---|---|
| Fixed-point iteration | Simple to write; can reuse an explicit temperature update | Can oscillate or diverge under strong latent/radiative coupling; stopping criteria can conceal non-convergence | Reject for first authoritative solve |
| Newton | Fast near a smooth root | Needs derivative or finite-difference policy; can leave physical domain; derivative becomes unreliable around availability/status boundaries | Defer |
| Secant | No analytic derivative | Still can leave bracket; two-point history complicates deterministic failure handling | Defer |
| Bisection | Bracketed, deterministic, derivative-free, explicit failure semantics | More residual evaluations than Newton/Secant | **Select** |
| Brent-Dekker | Bracketed and faster than pure bisection | More implementation surface and branch behavior; no product need for the extra complexity yet | Defer until profiling proves necessary |

The implementation should first evaluate both bracket endpoints. If either residual is
non-finite or required state is unavailable, return the corresponding explicit status.
If the finite residuals have no sign change, return `solver_no_bracket`; do not clamp
the result to an endpoint and call it converged. Each iteration chooses the midpoint,
retains the half-interval containing the sign change, and terminates only when both
temperature interval width and residual criteria are met.

## 10. Selected solver architecture

### 10.1 Search bounds

Use broad **numerical search bounds**, not Cannabis biological limits. The proposed
initial policy is an absolute engineering bracket of `233.15 K` through `373.15 K`
(`-40 degC` through `100 degC`) intersected with a configurable air-relative bracket
of `T_air - 80 K` through `T_air + 80 K`. The absolute bounds prevent invalid Kelvin
values and the relative bounds limit pathological input amplification. These numbers
are `NUMERICAL_POLICY`, not plant-survival claims, and must be visible in diagnostics.

If a future supported scenario needs a different numerical domain, it requires a model
registry/version change and regression scenarios. No Cannabis “safe temperature” is
derived from this bracket.

### 10.2 Tolerances and iterations

Initial implementation policy:

- maximum iterations: `32` (`NUMERICAL_POLICY`);
- temperature interval tolerance: `0.001 K` (`NUMERICAL_POLICY`);
- residual tolerance: `0.01 W m^-2` (`NUMERICAL_POLICY`, to be checked against
  reference-case scale before implementation acceptance);
- all comparisons use finite IEEE-754 `double` values on the supported platform;
- no wall-clock time, thread scheduling, or global random state enters the solve.

The tolerances are not scientific measurements and are not tuning knobs for gameplay.
They must be recorded in output with the solver/model version.

### 10.3 Failure and availability semantics

Proposed leaf-energy statuses:

```text
not_run
external_temperature_used
converged
missing_incident_shortwave
missing_radiative_surroundings_temperature
missing_leaf_energy_profile
missing_airflow
missing_leaf_characteristic_dimension
unsupported_boundary_layer
gas_exchange_unavailable
transpiration_unavailable
invalid_pressure
invalid_temperature_domain
non_finite_residual
solver_no_bracket
solver_iteration_limit
```

`converged` requires all of the following:

- the residual passed both convergence criteria;
- gas exchange returned an authoritative result;
- the shared boundary layer returned authoritative heat and vapor conductances;
- transpiration returned an authoritative result, including a genuine zero or a
  distinct condensation result;
- all reported energy components are finite.

An unavailable component is blank/unavailable in CSV and serialization. It is never
encoded as a calculated zero. A genuine calculated zero is emitted only with an
available status.

## 11. State and configuration architecture

### 11.1 Authoritative environmental inputs

Per spatial cell and segment:

```text
air_temperature_c
relative_humidity_percent
atmospheric_pressure_kpa
carbon_dioxide_umol_mol
airflow_m_s
ppfd_umol_m2_s
incident_shortwave_w_m2
radiative_surroundings_temperature_c
```

PPFD and incident shortwave are separate. Renderer lighting is not an input source.

### 11.2 Authoritative leaf/profile inputs

The future leaf-energy profile must contain:

```text
leaf_temperature_mode
shortwave_absorptance
longwave_emissivity
surface_factor
characteristic_leaf_dimension_m
effective_transpiring_leaf_area_m2
```

Each value must carry units, source/evidence class, calibration status, valid range,
and model/profile version. `surface_factor` is geometry/configuration, not a hidden
two-sided assumption.

The profile does not contain a gameplay “temperature comfort” constant. Growth,
stress, and consequences belong to P2.

### 11.3 Derived state

Per plant/cell/segment, derived state may include:

```text
leaf_temperature_c
leaf_temperature_source
leaf_vpd_kpa
absorbed_shortwave_w_m2
net_longwave_w_m2
sensible_heat_flux_w_m2
latent_heat_flux_w_m2
energy_residual_w_m2
solver_iterations
leaf_energy_status
leaf_energy_model_version
```

The existing environment and gas-exchange state remain authoritative in their current
domains. No generic health scalar is introduced.

### 11.4 External versus solved temperature modes

Use an explicit enum in scenario/profile configuration:

```text
EXTERNAL_LEAF_TEMPERATURE
SOLVED_LEAF_ENERGY_BALANCE
UNAVAILABLE
```

Semantics:

- `EXTERNAL_LEAF_TEMPERATURE`: a supplied measured/fixture leaf temperature is used
  by the accepted P1B.4.1 pipeline. The energy solver is bypassed and energy
  diagnostics are unavailable or marked `external_temperature_used`; the value is
  never silently overwritten.
- `SOLVED_LEAF_ENERGY_BALANCE`: a supplied external value is not used as the solver
  result. All required radiation, leaf profile, airflow, and accepted gas/
  transpiration inputs are required. Solver failure propagates as unavailable.
- `UNAVAILABLE`: no leaf VPD, transpiration, or water withdrawal may be declared
  authoritative from a missing temperature.

There is no implicit `AUTO` precedence rule. Backward-compatible scenarios that
already supply `leaf_temperature_c` should be migrated explicitly as
`EXTERNAL_LEAF_TEMPERATURE` in the implementation sprint.

### 11.5 Serialization and CLI consequences

The future save format must serialize mode, configured inputs, model versions, solver
policy, and authoritative solved/external state. Derived values may be serialized for
inspection but must be recomputable and must not become a second source of truth.
Save/load tests must prove that resuming at a segment boundary reproduces the same
leaf-energy result and downstream water request.

The future CLI time series should add stable columns:

```text
leaf_temperature_mode,leaf_temperature_source,
incident_shortwave_w_m2,radiative_surroundings_temperature_c,
absorbed_shortwave_w_m2,net_longwave_w_m2,sensible_heat_flux_w_m2,
latent_heat_flux_w_m2,energy_residual_w_m2,leaf_energy_iterations,
leaf_energy_status,leaf_energy_model_version
```

Blank means unavailable; zero means an available calculated zero.

## 12. Proposed coupling with P1B.4.1

The eventual per-segment sequence is:

```text
1. Sample the segment's spatial-cell environment.
2. Select EXTERNAL, SOLVED, or UNAVAILABLE temperature mode.
3. For SOLVED mode, validate explicit shortwave energy, radiative surroundings,
   leaf profile, airflow/dimension, pressure, and current hydraulic state.
4. Set the bracket over numerical leaf-temperature bounds.
5. For each trial T_leaf:
   a. calculate leaf saturation vapor pressure and leaf VPD;
   b. call the existing FvCB/Medlyn gas-exchange solver with current beta and
      trial leaf temperature/VPD;
   c. call the shared boundary-layer solver for heat and vapor conductance;
   d. call the existing transpiration calculation;
   e. convert its available flux to latent heat;
   f. calculate absorbed shortwave, net longwave, sensible heat, and F(T_leaf).
6. Run deterministic bisection until the selected criteria pass.
7. Re-evaluate the accepted root once as the authoritative gas-exchange,
   boundary-layer, and transpiration state.
8. Generate the existing water request from that final available transpiration,
   with no withdrawal from unavailable physiology.
9. Emit causal diagnostics and retain exact segment/event boundaries.
```

Water events, light changes, and simulation-day boundaries remain the segmentation
authority. No root-zone inventory is updated during a trial solve. No simultaneous
root-zone/leaf-temperature fixed point is introduced.

## 13. Validation and benchmark plan

Tests must be written before implementation and must assert both values and validity
semantics. Suggested acceptance tests:

1. zero net-radiation equilibrium sanity case;
2. increased absorbed radiation increases solved leaf temperature with all other
   supported inputs fixed;
3. increased airflow reduces the leaf-air temperature difference where the selected
   boundary-layer model predicts that direction;
4. increased available transpiration increases latent cooling;
5. hydraulic limitation reduces transpiration, reducing latent cooling and tending
   to increase leaf temperature in an otherwise identical case;
6. changed radiative-surroundings temperature changes longwave exchange and solved
   temperature;
7. missing incident shortwave returns `missing_incident_shortwave` and blank energy
   outputs;
8. missing radiative surroundings returns `missing_radiative_surroundings_temperature`;
9. unsupported boundary-layer flow propagates `unsupported_boundary_layer` even if
   gas exchange converges, with blank leaf-energy/transpiration outputs and no water
   withdrawal;
10. repeated identical solves are bitwise-identical on the supported platform;
11. converged residual is below tolerance and all component units are consistent;
12. no-sign-change and iteration-limit cases are explicit failures;
13. negative leaf-to-air VPD/condensation is represented as a valid negative latent
   energy flux when the accepted transpiration pipeline supports it, while water
   withdrawal remains non-negative and separately accounted;
14. external measured-temperature mode bypasses the energy solver without overwriting
   the measurement;
15. PPFD alone cannot activate solved mode; no hidden PPFD-to-W/m2 conversion occurs;
16. exact light and water-event segmentation produces one solve per fixed-forcing
   segment;
17. serialization round trip preserves configuration, mode, solver version, and
   authoritative result;
18. a segment-boundary save/load continuation matches uninterrupted execution.

### 13.1 Reference cases

The implementation should include three classes of reference case:

- one published/example equilibrium case derived from `tealeaves` or an equivalent
  source, with all example values explicitly labeled as source-example inputs and
  not promoted to CannaVille defaults;
- one analytically simple case, such as a no-net-input case where all terms are
  constructed to cancel at a known temperature;
- one synthetic controlled-environment scenario varying light, airflow, and
  transpiration availability independently.

The published case checks equation wiring, the analytic case checks signs and units,
and the synthetic case checks causal monotonicity. No case should be tuned merely to
make a Cannabis target temperature pass. Tolerances must be stated per case and
linked to the model/version under test.

## 14. Performance analysis

The selected bisection policy has predictable cost. A 140 K initial interval with a
`0.001 K` width criterion needs at most about 18 halvings; the configured maximum of
32 also covers residual-based termination and future bracket-policy changes. Each
trial invokes gas exchange, boundary-layer, and transpiration calculations.

Engineering planning estimate per fixed-forcing segment, before profiling:

| Plant count | Approximate trial solves at 32 iterations | Product implication |
|---:|---:|---|
| 10 | 320 | comfortably suitable for headless and eventual device testing |
| 100 | 3,200 | likely suitable; measure on target mobile hardware |
| 1,000 | 32,000 | requires profiling, segment/event amortization, and likely batching |
| 10,000 | 320,000 | do not run naively every simulation tick; use time-scale/LOD policy after evidence |

These are estimates, not benchmark results. The first implementation should solve only
when forcing, hydraulic state, or relevant configuration changes at an exact segment
boundary. It should not solve every render frame. A later performance pass may use a
faster bracketed method or cached environmental-cell results, but must preserve
determinism and validity semantics.

## 15. Scientific evidence and calibration gaps

The following are not established CannaVille production constants:

| Input/decision | Status | Why it remains open |
|---|---|---|
| `alpha_sw` broadband leaf absorptance | `CALIBRATION_REQUIRED` | Depends on leaf optical properties, spectrum, and orientation |
| PAR/NIR split | `OUT_OF_SCOPE` for P1B.5 | Not needed for the first explicit-energy architecture |
| `emissivity_lw` | `CALIBRATION_REQUIRED` | Leaf/cultivar/surface conditions and measurement convention |
| one- versus two-sided surface factor | `MODEL_CHOICE_REQUIRED` per leaf profile | Must match effective area and stomatal-area semantics |
| radiative-surroundings temperature | `CALIBRATION_REQUIRED` | Facility walls, fixtures, sky-equivalent surfaces, and geometry are not one universal value |
| leaf-level airflow mapping | `CALIBRATION_REQUIRED` | Room/cell airflow is not automatically local leaf velocity |
| heat boundary-layer property/conductance mapping | `MODEL_CHOICE_REQUIRED` | Must be reconciled with the accepted vapor conductance convention and supported Re domain |
| humid-air density and heat capacity policy | `MODEL_CHOICE_REQUIRED` | Choose and register a consistent air-property convention |
| latent-heat reference temperature | `MODEL_CHOICE_REQUIRED` | Leaf-temperature evaluation is recommended but must be validated against flux convention |
| numerical bracket, tolerance, and iteration cap | `NUMERICAL_POLICY` | Engineering convergence policy, not biology |
| Cannabis-specific leaf energy parameters by cultivar/stage | `CALIBRATION_REQUIRED` | Direct controlled-environment evidence is sparse and condition-bound |
| transient storage parameters | `OUT_OF_SCOPE` | Dynamic model is intentionally deferred |

The Cannabis studies found in this research are useful for later calibration but do
not support universal Cannabis temperature limits or energy coefficients. For example,
Chandra et al. report PPFD/temperature/CO2 responses for their tested *C. sativa*
material and conditions; Arve et al. report controlled lighting and leaf gas-exchange
measurements for a specific cultivar and protocol. Neither is permission to turn a
single treatment value into a universal game constant.

## 16. Scientific governance and registry plan

No production model-registry entry is added in P1B.5R because no equation is
implemented. The next implementation sprint must add entries before code becomes
authoritative. Proposed identifiers are:

```text
leaf_energy_balance.steady_state.v1
leaf_energy_balance.shortwave_explicit.v1
leaf_energy_balance.longwave_surroundings.v1
leaf_energy_balance.sensible_heat_shared_boundary_layer.v1
leaf_energy_balance.latent_heat_from_p1b41_flux.v1
leaf_energy_balance.bisection_policy.v1
```

Each entry must contain the repository schema's model ID/version, purpose, exact
equation or algorithm, inputs and outputs, units, source title/authors/organization,
DOI or stable identifier, valid domain, assumptions, approximation status, calibration
status, implementation path, validation tests, and notes. Constants must separately
record name, value, units, source, calibration state, valid range, uncertainty when
known, and evidence classification.

The registry must classify items using the existing governance vocabulary and the
following explicit extensions where useful:

```text
PHYSICAL_CONSTANT
PEER_REVIEWED_GENERAL_PLANT
CONTROLLED_ENVIRONMENT_GENERAL
CANNABIS_SPECIFIC
CALIBRATION_REQUIRED
SYNTHETIC_TEST_VALUE
NUMERICAL_POLICY
EMPIRICAL_APPROXIMATION
```

Gameplay balancing must not modify these scientific constants. No random process is
needed by the leaf-energy solver; if future uncertainty is introduced, it must be
seeded, isolated, and documented as uncertainty rather than used to replace a missing
causal mechanism.

## 17. Explicit exclusions

P1B.5 implementation must not include:

- plant growth, biomass, yield, or quality;
- morphology, canopy shading, multi-leaf ray tracing, or spectral ray tracing;
- HVAC, room heat balance, room humidity feedback, or CFD;
- root temperature physics, xylem networks, or nutrient effects;
- pests, pathogens, treatments, or disease pressure;
- ABA signaling or a new stress accumulator;
- Cannabis “optimal” temperature/VPD ranges;
- renderer brightness as horticultural radiation;
- dynamic leaf thermal storage in the first implementation;
- a second transpiration, stomatal, or root-zone model.

## 18. Independent critical review and resolution

This artifact was reviewed once after drafting against the requested closure criteria.

### Scientific defensibility

**Finding:** A one-way leaf-temperature update would hide the circular dependence of
leaf VPD, stomata, transpiration, and latent cooling.
**Resolution:** The contract requires a scalar residual root and a final authoritative
re-evaluation.

**Finding:** PPFD-to-energy conversion is spectrum-dependent.
**Resolution:** Incident shortwave energy is explicit; no default conversion is
permitted.

**Finding:** Dynamic storage is scientifically real.
**Resolution:** It is documented as a deliberate steady-state approximation with a
segment-duration caveat and deferred storage model, not omitted silently.

### Units and boundaries

**Finding:** Existing P1B.4.1 conductance is molar vapor conductance while standard
energy equations often use velocity conductance.
**Resolution:** The future shared boundary-layer API must return both vapor and heat
conductances with explicit conversions and registry entries.

**Finding:** A converged gas-exchange solution does not prove transpiration or energy
validity.
**Resolution:** Integrated leaf-energy validity requires authoritative heat boundary
conductance and transpiration; unsupported/unavailable states remain blank and cannot
generate water withdrawal.

### Determinism and implementation feasibility

**Finding:** Newton/fixed-point methods can leave the physical domain or fail at status
boundaries.
**Resolution:** Deterministic bisection with explicit bracket and iteration failures
is selected for the first implementation.

**Finding:** Solving 10,000 plants naively per tick would be disproportionate.
**Resolution:** Solve at exact forcing/state boundaries, estimate cost before coding,
and defer batching/LOD to an evidence-based performance pass.

### Product proportionality and scope

**Finding:** Full radiation transport, dynamic leaf tissue, HVAC, and canopy models
would turn this sprint into a research platform.
**Resolution:** The contract stops at single-leaf steady-state energy feedback and
explicitly marks the rest out of scope. The roadmap pivot to P2 is recorded.

No unresolved HIGH or CRITICAL design defect remains in this research artifact.
Remaining open items are explicitly classified `CALIBRATION_REQUIRED`,
`MODEL_CHOICE_REQUIRED`, `NUMERICAL_POLICY`, or `OUT_OF_SCOPE` and are not disguised
as settled science.

## 19. P1B.5 implementation contract

The next sprint may implement only the following boundary:

```text
per fixed segment and plant:
  explicit environment/radiation inputs
  + leaf energy profile
  + beginning-of-segment hydraulic beta/state
  -> deterministic bounded steady-state T_leaf solve
  -> shared boundary-layer and existing P1B.4.1 transpiration result
  -> latent/sensible/radiative diagnostics
  -> leaf VPD and water request only when integrated validity is authoritative
```

It must not modify the accepted P1B.4.1 equations. Any needed boundary-layer API
extension must preserve the accepted vapor result, statuses, spatial sampling,
segmentation, conservation ledger, and save/load behavior. The implementation sprint
must add registry entries before production equations, write tests before tuning,
and stop for one closure review before merge.

## 20. Post-P1B.5 roadmap pivot

After P1B.5 implementation and one closure review, foundational physiology pauses.
The next major program is:

```text
P2 — Plant Growth and Gameplay Consequence Layer

environment + physiology
  -> time-integrated plant opportunity/stress
  -> growth-stage progression
  -> visible vigor / morphology proxies
  -> stress symptoms
  -> player diagnosis
  -> intervention
  -> recovery / damage
  -> yield + quality consequences
```

P2 is not implemented or designed in detail here. This explicit pivot is a product
governance decision intended to prevent additional foundational-simulation drift.
