# P2.0R — Plant Growth and Gameplay Consequence Architecture

Status: research/design authority for the next implementation sprint
Baseline: v0.7.0-p1b5-leaf-energy-balance / 125d187b1b849e39529616b9d6846bf1a28de230
Production code changed in this sprint: none
Decision: GO for the bounded P2.0 implementation contract below

## 1. Executive recommendation

Implement a small, channelized consequence layer. It must not become a crop model or
replace P1 physiology.

The minimum useful model is:

1. four configured development stages: seedling, vegetative, flowering, and
   harvest_ready;
2. stage progress driven by an accumulated relative growth opportunity signal rather
   than raw net assimilation or a universal calendar;
3. three bounded exposure channels supported by current P1 state: water, thermal, and
   radiation;
4. per-channel exposure dose, residual recovery, and persistent damage accumulators;
5. a separate monotonic lost-opportunity accumulator;
6. derived symptoms, reason codes, and a player-condition presentation enum;
7. deterministic segment integration and save/load continuation.

The layer must never introduce an authoritative health scalar. A player's simple
condition label is a projection over decomposed state and cannot be biological source
of truth.

This makes two genetically identical plants with different management histories
visibly and persistently different: they can have different achieved opportunity,
lost opportunity, exposure dose, recovery state, and channel-specific damage even when
their current environment becomes identical.

## 2. Product objective

P1 already provides causal environmental and short-term physiological state. P2 turns
that state into a legible management loop:

~~~
P1 environment and physiology
        ↓
segment-level opportunity and exposure integration
        ↓
latent history: progress, dose, recovery, damage, lost opportunity
        ↓
derived symptoms and reason codes
        ↓
player observes, diagnoses, and changes management
~~~

A plant may survive and recover while still having less future opportunity than a plant
managed well throughout. The goal is causal persistence and comprehensibility, not
biological completeness.

## 3. Accepted P1 input inventory

This inventory is taken from the accepted production state. P2 must consume these
interfaces and must not silently reinterpret them as new physiology.

| P1 input | Timing and units | Authority in P2 | Use |
|---|---|---|---|
| simulation clock elapsed | cumulative simulation seconds | authoritative clock | segment time and ordering |
| exact segment duration | seconds | authoritative integration interval | all P2 integration |
| existing latent growth stage | categorical | bootstrap placeholder | initial stage only; P2 owns governed development |
| net assimilation | micromol CO2 m^-2 s^-1, instantaneous segment result | authoritative when P1 status is valid | relative opportunity only; never biomass |
| stomatal conductance | mol m^-2 s^-1, instantaneous | diagnostic/causal evidence | explanation support |
| hydraulic beta | dimensionless, segment-start state | authoritative when available | water exposure evidence |
| gas exchange and transpiration status | categorical | availability gate | prevents inferred biology from unavailable state |
| requested, realized, and unmet water | segment/cumulative ledger quantities | authoritative allocation outcome | shortage evidence and opportunity cap |
| root-zone storage, VWC, matric potential | explicit P1 units | physical/diagnostic state | instrumentation and water evidence |
| leaf temperature | optional degree C | external or P1B.5 solved result | thermal exposure input |
| leaf VPD | optional kPa | derived P1 measurement | diagnostic support, not another P2 stress channel |
| incident or absorbed shortwave | optional W m^-2 | P1B.5 result when available | radiation exposure input |
| PPFD and DLI | micromol photons m^-2 s^-1; mol m^-2 d^-1 | authoritative lighting state | context; no hidden energy conversion |
| light and dark duration | seconds | authoritative lighting bookkeeping | stage-gate evidence |
| air temperature, RH, pressure, CO2, airflow | explicit P1 units | instrumentation/context | supporting variables and later calibration |
| simulation version, seed, fixed timestep | configuration | determinism metadata | reproducibility and save/load |
| segment trace | per-segment engineering diagnostics | diagnostic only | tests and explanations, never second truth |

Unavailable P1 values are not silently treated as zero, normal, or unstressed. P2
must carry an explicit availability/status result for every channel it integrates.

The current displayed growth stage and readiness label are bootstrap placeholders, not
evidence that a functioning growth model already exists.

## 4. Selected lifecycle model

### Recommendation

Use an explicit cultivar/scenario development profile:

~~~
SEEDLING → VEGETATIVE → FLOWERING → HARVEST_READY
~~~

Each transition is governed by a configured required opportunity budget for the current
stage. An optional minimum elapsed time and optional photoperiod gate may also be
configured. The default does not invent a universal photoperiod threshold or stage
duration.

Stage progress is:

~~~
stage_progress = achieved_stage_opportunity / required_stage_opportunity
~~~

clamped to [0, 1]. Transition occurs only when the budget and explicit gates pass.
Stress can therefore delay a transition by reducing achieved opportunity, while improved
management can still recover current symptoms.

Chronological time alone cannot express different management histories. Thermal time
requires cultivar/stage-specific temperature thresholds that P1 does not provide.
An opportunity budget is transparent for a game and can later be calibrated without
changing the presentation contract. It is not a universal Cannabis phenology law.

Photoperiod remains an optional configured transition condition because Cannabis
flowering response is cultivar-dependent. It is never silently applied to every
cultivar.

Lifecycle parameters are CALIBRATION_REQUIRED:

- required opportunity per stage;
- optional minimum stage time;
- optional photoperiod/light-dark gate;
- auto-flowering, photoperiod-sensitive, or explicit-scenario behavior;
- starting stage and stage progress.

The state machine and serialization contract are RESOLVED_FOR_P2.

## 5. Selected growth-opportunity model

### Opportunity is not biomass

P1 net assimilation is a leaf-area-normalized instantaneous gas-exchange output. It is
not whole-plant carbon gain, biomass, yield, or harvest quality. P2 uses it only to
form a relative opportunity signal and must not emit grams, biomass, yield, or
cannabinoid output.

### Relative opportunity signal

For a valid segment, use a configured stage/cultivar reference with units
micromol CO2 m^-2 s^-1:

~~~
assimilation_fraction = clamp(A_net / A_reference, 0, 1)
~~~

Negative net assimilation produces zero opportunity for this relative signal; it is not
relabeled as a measured respiration or biomass rate by P2.

When a valid request and allocation outcome exist:

~~~
water_fulfilment = clamp(realized_request / requested_request, 0, 1)
~~~

Water fulfilment is 1 when the request is zero. The P2 v1 policy is:

~~~
opportunity_signal = min(assimilation_fraction, water_fulfilment)
~~~

The water term caps an observed shared-allocation shortage. It is not an additional
multiplicative drought penalty. Hydraulic beta already influences P1 gas exchange, so
P2 must not multiply the same shortage again through an arbitrary stress factor.

If required P1 inputs are unavailable, the result is opportunity_unavailable. P2 does
not guess a normal value. Non-biological fixtures must declare SYNTHETIC_TEST_VALUE.

### Opportunity integrals

For each fixed-forcing segment of duration dt seconds:

~~~
potential_opportunity_s += dt
achieved_opportunity_s += opportunity_signal * dt
lost_opportunity_s += (1 - opportunity_signal) * dt
~~~

These are relative opportunity-seconds, not carbon or biomass. The identity

~~~
potential_opportunity_s = achieved_opportunity_s + lost_opportunity_s
~~~

must hold within configured floating-point tolerance. Lost opportunity is monotonic
and does not reset when conditions improve.

P2 uses assimilation as the primary opportunity signal. It does not apply a separate
thermal or radiation growth multiplier in v1, avoiding double-counting consequences
already reflected in P1 assimilation or water fulfilment. Thermal and radiation
channels remain independently useful for symptoms and persistent consequences.

## 6. Stress/exposure integration

P2 distinguishes:

1. current severity: adverse forcing in the current P1 segment;
2. exposure dose: severity accumulated over time;
3. persistent damage: bounded consequence after a channel-specific damage policy.

The independent channels are water, thermal, and radiation. Each severity is
dimensionless in [0, 1] with a channel-specific status.

### Water

When beta is authoritative:

~~~
water_limitation_from_beta = clamp(1 - beta_hydraulic, 0, 1)
~~~

When request and allocation are valid:

~~~
water_shortage = clamp(unmet_request / requested_request, 0, 1)
~~~

Integrated water severity is the maximum of these evidence channels, not their sum.
This avoids counting the same shortage twice. If neither evidence source is valid,
water exposure is unavailable.

### Thermal

Thermal severity is derived from accepted leaf temperature using a configured profile
with onset, full-severity, and optional critical thresholds. The mapping is monotonic
and clamped. Thresholds are not Cannabis defaults and are CALIBRATION_REQUIRED.

P2 v1 selects one bounded heat-exposure band. No universal ideal Cannabis temperature
is introduced.

### Radiation

Radiation severity uses explicit P1B.5 absorbed shortwave energy when available. It
is never inferred from renderer brightness or a hidden PPFD conversion. A configured
absorbed-energy onset/full-severity profile is required and is CALIBRATION_REQUIRED.

The initial channel represents excessive radiation load only. Low light is represented
through opportunity, not mislabeled as radiation damage.

### Dose

For channel c and segment duration dt:

~~~
exposure_dose_c_s += severity_c * dt
damage_dose_c_s += max(0, severity_c - damage_onset_c) * dt
~~~

The damage onset and conversion from damage dose to persistent damage are
channel-specific configuration. The first implementation may use:

~~~
persistent_damage_c = clamp(damage_dose_c / damage_scale_c, 0, 1)
~~~

This is a transparent deterministic gameplay accumulator, not a cellular injury law.
All rates, thresholds, and dose scales are CALIBRATION_REQUIRED unless explicitly
declared SYNTHETIC_TEST_VALUE.

## 7. Recovery model

Recovery is separate from current stress and persistent damage. It expresses residual
consequence after forcing improves.

For channel c, with configured recovery rate k_c in s^-1:

~~~
recovery_c_next =
    recovery_c * exp(-k_c * dt)
    + severity_c * (1 - exp(-k_c * dt))
~~~

The result is clamped to [0, 1]. This deterministic closed-form update does not
depend on render frames and makes a short stress episode recoverable without erasing
dose.

Recovery rates are not universal biology. They are channel/stage/cultivar calibration
parameters. Tests must label fixture values SYNTHETIC_TEST_VALUE until calibrated.

When forcing ends, current severity falls, recovery declines gradually, historical dose
remains, persistent damage remains unless a later model permits repair, and lost
opportunity never decreases.

## 8. Persistent consequence/damage model

P2 v1 stores separate damage channels:

~~~
water_damage_0_1
thermal_damage_0_1
radiation_damage_0_1
~~~

There is no aggregate authoritative health value. A presentation layer may compute a
maximum or weighted summary, but that summary is not biological truth.

Persistent damage is deliberately coarse. It represents a continuing penalty to future
opportunity or appearance, not a specific organ, hormone, pathogen, or tissue lesion.
P2 does not create chlorosis, necrosis, nutrient deficiency, or disease symptoms
because those causes are not modeled.

## 9. Lost-opportunity model

Lost opportunity is the central survive-but-do-not-instantly-catch-up mechanic. It is
the cumulative gap between potential and achieved opportunity, and is different from
damage:

- a plant can lose opportunity with no persistent damage;
- damage can remain after current stress declines;
- correcting conditions improves future opportunity but cannot erase history;
- identical current P1 state can coexist with different lost opportunity.

A future yield/quality layer may consume stage-specific lost opportunity, especially
flower-stage lost opportunity, but P2 does not assign a yield equation.

## 10. Observable symptoms

P2 v1 exposes only symptoms linked to modeled causes.

| Observable | Derived from | Semantics |
|---|---|---|
| wilt_severity_0_1 | current/recovering water channel | water-limitation appearance; no pathogen/nutrient implication |
| growth_slowdown_0_1 | 1 minus opportunity plus stage-progress lag | relative development consequence |
| heat_stress_severity_0_1 | current/recovering thermal channel | heat-load presentation |
| radiation_stress_severity_0_1 | current/recovering radiation channel | excessive-light presentation |
| damage_visual_0_1 | channel damage projection | persistent appearance proxy, not health |
| recovery_severity_0_1 | residual recovery channels | forcing improved but plant is not fully recovered |

Symptoms are derived and may use presentation hysteresis to avoid flicker. Hysteresis,
onset delays, notification timing, and visual mapping are GAMEPLAY_PRESENTATION, not
scientific constants. P2 does not emit nutrient-specific yellowing or pathogen lesions.

## 11. Latent, visible, and instrumented state

### Latent authoritative state

~~~text
PlantConsequenceState
  model_version
  stage
  stage_progress_0_1
  stage_opportunity_accumulated_s
  potential_opportunity_s
  achieved_opportunity_s
  lost_opportunity_s
  current_opportunity_signal_0_1
  water_exposure_dose_s
  thermal_exposure_dose_s
  radiation_exposure_dose_s
  water_recovery_0_1
  thermal_recovery_0_1
  radiation_recovery_0_1
  water_damage_0_1
  thermal_damage_0_1
  radiation_damage_0_1
  integration_status
  last_integrated_simulation_time_s
~~~

Opportunity and dose durations are seconds; fractions are dimensionless; stage is an
enum; time is simulation seconds. No generic health field is permitted.

### Visible derived state

~~~text
plant_id
as_of_simulation_time_s
state_version
growth_stage
stage_progress_0_1
size_proxy
vigor_signal
wilt_severity_0_1
growth_slowdown_0_1
heat_stress_severity_0_1
radiation_stress_severity_0_1
damage_severity_0_1
recovery_severity_0_1
derived_condition
reason_codes[]
~~~

Size_proxy and vigor_signal are presentation proxies, not meters or biomass. A future
UE adapter may consume this contract without owning the biology.

### Instrumented state

Numerical instruments may expose VWC, matric potential, beta, leaf temperature, VPD,
PPFD, DLI, assimilation, stomatal conductance, airflow, and water ledger outcomes.
Instrumented state is not automatically visible to the player.

## 12. Causal reason-code architecture

The simulator, not an AI classifier, produces machine-readable evidence. Each reason
record contains:

~~~text
reason_code
severity_0_1
start_time_s
duration_s
supporting_variables[]
source_channel
status
~~~

Initial codes:

~~~text
WATER_SUPPLY_LIMITED
HIGH_LEAF_TEMPERATURE
HIGH_RADIATION_LOAD
LOW_GROWTH_OPPORTUNITY
RECOVERING_FROM_WATER_STRESS
RECOVERING_FROM_THERMAL_STRESS
RECOVERING_FROM_RADIATION_STRESS
PERSISTENT_WATER_DAMAGE
PERSISTENT_THERMAL_DAMAGE
PERSISTENT_RADIATION_DAMAGE
STAGE_DELAYED_BY_LOST_OPPORTUNITY
P2_INPUT_UNAVAILABLE
~~~

Examples of evidence include beta, requested/realized/unmet water, leaf temperature,
absorbed shortwave, assimilation fraction, stage opportunity, and the exact interval.
Unavailable input produces an unavailable-status record, not a confident cause.

## 13. Derived player-condition model

The UI may derive:

~~~text
UNAVAILABLE
THRIVING
STABLE
STRESSED
RECOVERING
DAMAGED
CRITICAL
~~~

Precedence is explicit:

1. UNAVAILABLE if required evidence is unavailable;
2. CRITICAL if active channel or damage reaches the configured critical band;
3. DAMAGED if persistent damage exceeds its presentation threshold;
4. RECOVERING if forcing is below onset but residual recovery is visible;
5. STRESSED if current exposure or opportunity deficit exceeds onset;
6. THRIVING or STABLE according to opportunity and low-channel thresholds.

Thresholds are GAMEPLAY_PRESENTATION. The label is never fed back into integration
and cannot replace decomposed state.

## 14. UE-facing future state contract

This is a data contract only, not an Unreal class design. The future adapter should
query the versioned observable projection shown in the visible-state block above.
It may map these values to meshes, materials, animation, camera/UI, and player feedback.
It must not calculate stage, stress, damage, or reason codes.

## 15. Yield/quality future interface

P2 does not implement yield, quality, cannabinoids, terpenes, harvest chemistry, or
market value. It leaves only bounded historical inputs for later:

~~~text
achieved_opportunity_s
lost_opportunity_s
stage-specific achieved/lost opportunity
flower-stage severe exposure dose
water/thermal/radiation persistent damage
harvest-ready transition time
~~~

A later yield/quality model must declare its own sources. It must not interpret lost
opportunity as grams or a fixed percentage yield loss without a separate registry entry.

## 16. Calibration boundaries

Biological calibration required:

- stage opportunity budgets;
- optional minimum stage durations;
- cultivar-specific photoperiod gates or auto-flowering behavior;
- stage/cultivar assimilation reference;
- water, thermal, and radiation onset/full/critical bands;
- recovery rates or half-lives by channel and stage;
- damage onset and dose scales;
- damage repair policy, if introduced;
- stage-specific effect of lost opportunity;
- any later relationship between P2 history and yield/quality.

General plant evidence supports the design principle that stress magnitude, duration,
sequence, developmental age, and recovery history matter. It does not supply universal
values for CannaVille cultivars, indoor facilities, or game timescale.

Gameplay presentation calibration includes symptom/hysteresis thresholds, condition
labels, reason visibility, notification timing, visual mapping, player knowledge, and
sensor disclosure.

Every future parameter must be classified as:

~~~text
CANNABIS_SPECIFIC
GENERAL_PLANT
CALIBRATION_REQUIRED
SYNTHETIC_TEST_VALUE
GAMEPLAY_PRESENTATION
~~~

No synthetic fixture may become biological truth.

## 17. Deterministic integration semantics

P2 integrates only at accepted P1 fixed-forcing segment boundaries. Exact water events,
light transitions, and simulation-day boundaries already segment P1; P2 consumes the
same segments.

For each segment:

~~~text
1. P1 resolves environment, leaf energy, gas exchange, transpiration, and allocation.
2. P1 exposes the final accepted segment result and status.
3. P2 validates availability and computes channel severities.
4. P2 computes relative opportunity without mutating P1.
5. P2 updates potential, achieved, and lost opportunity.
6. P2 updates dose, recovery, and persistent damage.
7. P2 advances stage progress and explicit transition gates.
8. P2 derives symptoms, reasons, and player condition.
9. P2 serializes authoritative state at the segment boundary.
~~~

Policies:

- dt is simulation seconds and must be finite and non-negative;
- bounded severities and fractions are clamped to [0, 1] after validation;
- no render-frame or wall-clock update exists;
- no random draw is used for stress, damage, or recovery;
- unavailable inputs yield explicit status and do not become normal values;
- stable plant ID order is used for transitions;
- reason arrays use stable ordering;
- identical scenario, configuration, version, seed, timestep, and duration produce
  identical output on the supported platform;
- opportunity identity is checked independently;
- P2 does not mutate root-zone inventory or rerun water allocation.

P2 is compatible with offline progression because unchanged intervals can be advanced
as deterministic coarse segments. It does not require animation, rendering, or UI.

## 18. Serialization implications

Persist:

- P2 model version and profile IDs;
- stage and stage-local progress;
- opportunity accumulators;
- per-channel exposure doses;
- per-channel recovery;
- per-channel persistent damage;
- integration status and last integrated simulation time;
- calibration/profile version identifiers.

Derived symptoms, condition labels, and reason records may be recomputed after load.
Cached derived values are non-authoritative and must be validated. Temporary
intermediate values are not authoritative save state.

Save/load at a segment boundary must produce the same next P1/P2 result as a
straight-through run. A model-version mismatch must fail explicitly or use a governed
migration; it must not reinterpret historical dose silently.

## 19. Acceptance tests for P2 implementation

The next sprint must add controlled tests for:

1. favorable management: high achieved opportunity, low exposure, no persistent damage;
2. short water limitation: exposure, visible/recovering consequence, eventual current recovery;
3. repeated water limitation: more dose, slower progress, greater consequence than short;
4. high absorbed radiation/leaf temperature: configured thermal/radiation consequence;
5. identical final forcing but different histories: different lost opportunity/recovery/damage;
6. recovery: current stress declines but history does not reset;
7. opportunity identity after every segment;
8. deterministic stage transition gates with no universal Cannabis timing;
9. unavailable P1 inputs produce explicit status and no guessed exposure;
10. reason codes identify supporting variables and source channel;
11. derived condition changes without becoming authoritative state;
12. save/load matches straight-through output;
13. repeated identical runs are value/byte identical under platform contract;
14. a 7–14 day scenario separates three management trajectories;
15. no nutrients, pests, disease, yield, quality, or new P1 physiology appear.

## 20. 7–14 day gameplay scenario

Use a future headless fixture of 10 simulated days with three genetically identical
plants and the same stage/calibration profile.

GOOD_MANAGEMENT: valid P1 inputs, adequate allocation, supported airflow, moderate
absorbed shortwave, and no exposure threshold crossing. Expected: highest achieved
opportunity and stage progress, near-zero dose, no persistent damage, THRIVING or STABLE.

TEMPORARY_MISTAKE_AND_RECOVERY: bounded water shortage and elevated radiation/leaf
temperature for an early window, then favorable forcing. Expected: more early exposure
and lost opportunity than good management, symptoms during correction, reduced stage
progress, and less persistent damage than chronic poor management. Final forcing can
match the good plant while history differs.

CHRONIC_POOR_MANAGEMENT: repeated water limitation and excessive radiation/thermal
load with no sufficient recovery interval. Expected: lowest achieved opportunity and
stage progress, greatest lost opportunity and persistent damage, and DAMAGED or
CRITICAL according to presentation thresholds.

Required qualitative ordering:

~~~text
achieved opportunity: GOOD > TEMPORARY_RECOVERY > CHRONIC
stage progress:       GOOD > TEMPORARY_RECOVERY > CHRONIC
lost opportunity:     GOOD < TEMPORARY_RECOVERY < CHRONIC
persistent damage:    GOOD < TEMPORARY_RECOVERY < CHRONIC
~~~

Exact numeric values are not acceptance targets until calibrated.

## 21. Explicit exclusions

This sprint does not authorize or implement:

- new photosynthesis, stomatal, hydraulic, transpiration, or leaf-energy equations;
- thermal-time equations as a default Cannabis model;
- nutrient, pH, EC, oxygen, or root-temperature response;
- pests, pathogens, latent infection, treatments, or resistance;
- morphology, canopy shading, plant architecture, or organ-level botany;
- biomass, yield, harvest chemistry, cannabinoids, terpenes, or quality;
- Unreal, rendering, 3D assets, animation, or UI implementation;
- workers, economy, market systems, or cloud services;
- random damage, random stress, or opaque AI inference;
- full crop models such as DSSAT/AquaCrop;
- a generic plant-health scalar.

## 22. P2 implementation contract

The next sprint may add only:

- versioned P2 configuration/profile structures;
- the selected stage state machine;
- relative opportunity and lost-opportunity integration;
- water, thermal, and radiation exposure/recovery/damage channels;
- derived symptoms, reasons, and condition projection;
- deterministic segment integration;
- serialization and save/load continuation;
- CLI diagnostics;
- the acceptance fixtures listed above.

It must preserve P1 as the scientific source of truth and keep biological and gameplay
parameters separately classified. It must not add a foundational physiology subsystem.

## 23. Post-P2 next step

After one implementation sprint and one closure review, decide whether players can
observe, diagnose, and change outcomes. The likely next layer is a player-facing
intervention and facility-management loop, not a return to foundational physiology.

P2 should expose enough state for future yield/quality interfaces, but those models
remain separately authorized and evidence-governed.

## Research evidence and design boundaries

The research ceiling was intentionally narrow. These sources support design constraints,
not universal CannaVille parameters.

| Source | Relevant evidence | Boundary |
|---|---|---|
| Zhang, Anderson, Brym and Pearson, Photoperiodic Flowering Response of Essential Oil, Grain, and Fiber Hemp, Frontiers in Plant Science 12 (2021), [doi:10.3389/fpls.2021.694153](https://doi.org/10.3389/fpls.2021.694153), [PMC](https://pmc.ncbi.nlm.nih.gov/articles/PMC8367441/) | Controlled photoperiod responses differed across cultivars; small changes altered flowering timing for some tested material | Supports configured/genotype-bound gates; no universal threshold |
| Moher, Jones and Zheng, Photoperiodic Response of In Vitro Cannabis sativa Plants, HortScience 56 (2021), [doi:10.21273/HORTSCI15452-20](https://doi.org/10.21273/HORTSCI15452-20) | Flower initiation was tested across photoperiods for genotype 802; whole-plant generalization remains open | Supports calibration-required photoperiod profiles |
| Fleming, Chamberlain, Zager and Lange, Controlled environments for cannabis cultivation to support omics research studies and production, Methods in Enzymology 680 (2023), [doi:10.1016/bs.mie.2022.07.028](https://doi.org/10.1016/bs.mie.2022.07.028) | Describes controlled-environment periods including vegetative growth, flowering, and harvest | Supports explicit lifecycle boundaries; not universal duration |
| Chandra et al., Photosynthetic response of Cannabis sativa L. to variations in PPFD, temperature and CO2 conditions, Physiology and Molecular Biology of Plants (2008), [PMC](https://pmc.ncbi.nlm.nih.gov/articles/PMC3550641/) | Cannabis gas exchange varied with PPFD, temperature, and CO2 under tested conditions | Supports P1 measurements as relative opportunity evidence; not whole-plant calibration |
| Daiber, Hopf, Luedeling and Hoogenboom, Process-based modeling of growth and development of cannabidiol-rich hemp cultivars grown in controlled environments, Acta Horticulturae 1425 (2025), [doi:10.17660/ActaHortic.2025.1425.44](https://doi.org/10.17660/ActaHortic.2025.1425.44) | Process-based growth modeling required cultivar/environment data; photoperiod adaptation and independent evaluation remained future work | Supports calibrated profiles and rejection of universal stage constants |
| Bonhomme, Bases and limits to using degree.day units, European Journal of Agronomy 13 (2000), [doi:10.1016/S1161-0301(00)00058-7](https://doi.org/10.1016/S1161-0301(00)00058-7) | Thermal-time units are useful for phase prediction but depend on defensible thresholds and interpretation | Supports deferring thermal time until cultivar parameters exist |
| Farooq et al., Plant drought stress: effects, mechanisms and management, Agronomy for Sustainable Development 29 (2009), [doi:10.1051/agro:2008021](https://doi.org/10.1051/agro:2008021) | Drought outcomes depend on severity and duration and affect growth, phenology, and physiology | Supports exposure dose; no CannaVille rates |
| Chaves, Flexas and Pinheiro, Photosynthesis under drought and salt stress, Annals of Botany 103 (2009), [PMC](https://pmc.ncbi.nlm.nih.gov/articles/PMC2707345/) | Recovery after stress is not necessarily instantaneous and can depend on severity and duration | Supports recovery/history separation; no P2 constants |
| Rankenberg et al., Age-Dependent Abiotic Stress Resilience in Plants, Trends in Plant Science 26 (2021), [doi:10.1016/j.tplants.2020.12.016](https://doi.org/10.1016/j.tplants.2020.12.016) | Developmental age changes abiotic stress susceptibility | Supports stage-specific calibration hooks |
| Akter et al., Mechanisms of plant acclimation to multiple abiotic stresses, Plant Communications (2025), [PMC](https://pmc.ncbi.nlm.nih.gov/articles/PMC12019247/) | Magnitude, order, and duration of combined stresses affect outcomes | Supports separate channels and no arbitrary additive damage |

### Research classifications

RESOLVED_FOR_P2:

- channelized state instead of generic health;
- explicit opportunity integral instead of raw assimilation-as-biomass;
- exposure dose separated from recovery and damage;
- exact fixed-segment integration;
- deterministic reason-code output;
- configured lifecycle profile with no universal Cannabis duration.

CALIBRATION_REQUIRED:

- cultivar/stage opportunity budgets;
- reference assimilation values;
- stress thresholds and dose scales;
- recovery rates and repair policy;
- persistent damage mapping;
- photoperiod gates;
- later yield/quality relationships.

DEFER_TO_LATER_GAMEPLAY:

- sensor ownership and fog-of-war disclosure;
- facility upgrades that reveal instrumentation;
- visual asset mapping and intervention UX;
- harvest timing choices beyond the stage contract.

OUT_OF_SCOPE:

- nutrients, pests, disease, treatments, morphology, canopy, new physiology,
  Unreal, cloud, yield chemistry, and market economy.

## Internal critical design review

High-risk findings resolved:

1. Generic health scalar risk: resolved by separate water, thermal, radiation,
   recovery, damage, opportunity, and lost-opportunity channels. Condition is derived.
2. Double-counting physiology: resolved by assimilation as primary opportunity, water
   shortage only as a cap, and thermal/radiation as separate consequence channels.
3. False Cannabis precision: resolved by calibration-governed parameters and no
   universal defaults.
4. History loss after recovery: resolved by monotonic lost opportunity and dose plus
   persistent channel damage.

Remaining non-blocking risks:

- P1 input unavailability must be surfaced clearly in implementation and CLI;
- presentation thresholds need playtesting without being labeled biology;
- initial calibration may support rankings, not absolute Cannabis growth prediction.

None blocks deterministic P2 implementation. Verdict: GO.

### Critical-review checklist

| Review question | Finding | Disposition |
|---|---|---|
| Causal clarity | P1 inputs flow through named opportunity/exposure channels into derived symptoms and reasons | resolved for P2 |
| Scientific overreach | Absolute growth, damage, and stage constants are not supported by the current evidence base | calibration required; no blocker |
| Hidden health scalar | No authoritative aggregate health field exists | resolved by channelized state |
| Double-counting stress | Assimilation is primary opportunity; water shortage is a cap; thermal/radiation are separate channels | resolved by explicit aggregation policy |
| Unit consistency | Durations use seconds, opportunity/dose are relative-seconds, severities/fractions are dimensionless, P1 quantities retain their units | resolved for implementation |
| History dependence | Lost opportunity, dose, recovery, damage, and stage progress persist across later identical forcing | resolved for implementation |
| Recovery semantics | Recovery decays after forcing improves; dose and lost opportunity do not reset | resolved for implementation |
| Determinism | Segment order, clamping, status propagation, and reason ordering are explicit; no new randomness | resolved for implementation |
| Calibration requirements | Biological thresholds, rates, budgets, and references are separately classified | resolved as a governance boundary |
| Player comprehensibility | Symptoms and machine-readable evidence are limited to modeled causes | resolved for P2 v1 scope |
| Implementation complexity | One state container, three channels, one opportunity integral, and derived projections are tractable for headless execution | no blocker |
| Product value | Water correction, airflow/light management, instrumentation, and recovery timing become consequential | supports GO |

## Product decision

### Minimum model for visibly different histories

Two genetically identical plants become persistently different when P2 stores:

~~~text
stage progress
+ achieved opportunity
+ monotonic lost opportunity
+ per-channel exposure dose
+ per-channel residual recovery
+ per-channel persistent damage
~~~

The plant with repeated poor management can recover current conditions while retaining
lower stage progress, greater lost opportunity, and greater damage than the plant
managed well. No generic health scalar is required.

### New meaningful player decisions

P2 makes these decisions consequential:

- correct a water shortage before exposure becomes persistent;
- improve airflow or radiation management when leaf temperature/load rises;
- invest in instrumentation before symptoms become obvious;
- accept lost growth opportunity or change the schedule/facility;
- determine whether an apparently recovered plant is ready to advance or harvest;
- protect a high-value flowering window rather than merely keep a plant alive.

These decisions are valuable because the simulator explains them with causal variables and
reason codes, not arbitrary damage.

Scope confirmation: No new foundational physiology was added or authorized.
