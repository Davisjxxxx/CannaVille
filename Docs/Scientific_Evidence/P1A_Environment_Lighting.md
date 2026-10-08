# P1A environmental physics and lighting evidence

P1A implements environmental measurements and bookkeeping only. It does not implement photosynthesis, transpiration, stomatal conductance, biomass, yield, stress, water uptake, or any other biological response.

## A. Established physical relationships

### Saturation vapor pressure

P1A registers `environment.saturation_vapor_pressure.fao56` and uses the FAO-56 equation:

```text
e_s(T) = 0.6108 * exp(17.27 * T / (T + 237.3))
```

where `T` is degrees Celsius and `e_s` is kilopascals. The implementation accepts `0 <= T <= 50 degC`, a deliberately documented controlled-environment domain. It rejects values outside that domain rather than selecting an ice-phase or alternative equation implicitly.

Source: [Allen, Pereira, Raes, and Smith, FAO Irrigation and drainage paper 56, Chapter 3](https://www.fao.org/4/X0490E/x0490e07.htm). FAO-56 identifies saturation vapor pressure as a temperature relationship, gives the equation, units, and reference values.

### Actual vapor pressure from RH

For an instantaneous measurement at one temperature:

```text
e_a = e_s * RH_percent / 100
```

Relative humidity is validated in the closed interval `[0, 100]` percent. Values outside the interval are rejected. The implementation does not silently clamp invalid sensor/configuration input.

FAO-56 documents RH-based actual vapor-pressure derivations and distinguishes actual vapor pressure from saturation vapor pressure in the same chapter. P1A uses the direct instantaneous relationship required by the scenario state; it does not pretend that daily RH extrema and instantaneous RH are interchangeable.

### Air VPD

```text
VPD_air = e_s(T_air) - e_a
```

The output is kilopascals. P1A exposes this as a physical environmental measurement only. No target band, plant multiplier, stress threshold, or cannabis-specific recommendation is implemented.

Source: [FAO-56 vapor-pressure-deficit calculation](https://www.fao.org/4/X0490E/x0490e07.htm), which defines VPD as the difference between saturation and actual vapor pressure.

### Leaf VPD

When leaf temperature is explicitly available:

```text
VPD_leaf = e_s(T_leaf) - e_a
```

When it is unavailable, leaf VPD is unavailable. Air temperature is never substituted. Negative leaf VPD is retained as a possible condensation condition instead of being silently clamped.

This is a physical extension of the sourced VPD definition to an explicit leaf-temperature input. It is not a cannabis calibration and is recorded as an engineering extension in the model registry.

### PPFD and DLI

PPFD is represented directly as micromoles of photons per square metre per second. It is supplied per spatial cell and is never derived from renderer brightness, electrical wattage, or an Unreal light actor.

DLI is accumulated from simulation time:

```text
DLI = integral(PPFD(t) * dt) / 1,000,000
```

The schedule implementation is exact for piecewise-constant scenario segments. Uncovered schedule intervals are dark. DLI resets at explicit 86,400-second simulation-day boundaries.

Sources:

- [Torres and Lopez, Measuring Daily Light Integral in a Greenhouse, Purdue University](https://ag.purdue.edu/department/hla/extension/extension-publications-library/ext-pubs/ho-238-w.html)
- [Stallknecht, Calculating and Using Daily Light Integral, Virginia Tech](https://www.pubs.ext.vt.edu/content/pubs_ext_vt_edu/en/SPES/spes-720/spes-720.html)

These sources establish the PPFD/DLI units and accumulation concept. They do not supply cannabis response constants.

### Pressure and CO2

P1A stores atmospheric pressure in kilopascals and CO2 in micromoles per mole per cell. Atmospheric pressure is not yet used in the RH-derived vapor-pressure identity. CO2 is not used by any biological model. Both are retained as explicit environmental measurements for later governed models.

### Rendering-light separation

Horticultural light state and rendering light state are separate architectural domains. P1A accepts physical PPFD as scenario data. No renderer, Unreal actor, shader brightness, lux conversion, fixture-optics model, or canopy-attenuation model exists in this branch.

## B. Cannabis-specific empirical evidence

These studies are evidence for later calibration and scenario design only. None is converted into a universal P1A constant.

### Rodriguez-Morrison, Llewellyn, and Zheng (2021)

[Cannabis Yield, Potency, and Leaf Photosynthesis Respond Differently to Increasing Light Levels in an Indoor Environment](https://doi.org/10.3389/fpls.2021.646020), *Frontiers in Plant Science* 12:646020. DOI: `10.3389/fpls.2021.646020`.

- Cultivar/genotype: *Cannabis sativa* ‘Stillwater’.
- Phase: two vegetative weeks at approximately 425 µmol m⁻² s⁻¹ and 18/6, followed by 12/12 flowering for 12 weeks.
- Flowering PPFD: canopy-level range approximately 120–1,800 µmol m⁻² s⁻¹.
- Environment reported in the trial: approximately 25.3 °C day / 25.2 °C night, RH approximately 60.5% day / 53.1% night, CO2 approximately 437 ppm day / 479 ppm night.
- Measured outcomes: leaf light-response behavior, inflorescence yield, harvest index, inflorescence density, cannabinoid potency, and terpene potency.
- Findings useful for later calibration: dry inflorescence yield and harvest index increased linearly over the tested canopy PPFD range; cannabinoid potency did not show a treatment effect; some terpene measures did.
- Limitations: one cultivar, one facility and production protocol, treatment-specific canopy measurements, and results that cannot be generalized to all genotypes or environments.
- P1A use: evidence that PPFD and accumulated light exposure must remain explicit state variables; no response equation is implemented.

### Collado, Hwang, and Hernández (2024)

[Supplemental greenhouse lighting increased the water use efficiency, crop growth, and cutting production in *Cannabis sativa*](https://doi.org/10.3389/fpls.2024.1371702), *Frontiers in Plant Science* 15:1371702. DOI: `10.3389/fpls.2024.1371702`.

- Cultivar/genotype: *Cannabis sativa* cv. ‘Suver Haze’.
- Phase: vegetative stock plants in a greenhouse.
- Lighting: supplemental LED levels approximately 150, 300, 500, and 700 µmol m⁻² s⁻¹ under an 18-hour photoperiod, combined with solar radiation.
- Reported DLI: approximately 17.9, 29.8, 39.5, and 51.8 mol m⁻² d⁻¹.
- Measured outcomes: biomass, leaf area, branching, crop evapotranspiration, water-use efficiency, leaf photosynthesis, and related gas-exchange measures including stomatal conductance.
- Findings useful for later calibration: light increases were associated with increased biomass and leaf area, increased evapotranspiration, and improved water-use efficiency across the tested treatment range.
- Limitations: vegetative greenhouse stock plants, one cultivar, combined sunlight and supplemental light, and controlled water/nutrient/genetic conditions. These results do not establish universal PPFD, DLI, transpiration, or WUE constants.
- P1A use: evidence that later models will need coupled light, water-use, and gas-exchange state; no coupling is implemented here.

### Magagnini, Grassi, and Kotiranta (2018)

[The Effect of Light Spectrum on the Morphology and Cannabinoid Content of *Cannabis sativa* L.](https://doi.org/10.1159/000489030), *Medical Cannabis and Cannabinoids* 1(1):19–27. DOI: `10.1159/000489030`.

- Plant material: cannabis clones; the article does not support treating the trial as a universal cultivar model.
- Phase: approximately 450 µmol m⁻² s⁻¹; 18/6 for 21 vegetative days and 12/12 for 46 generative days.
- Treatments: HPS, AP673L LED, and NS1 LED spectra.
- Measured outcomes: morphology, dry-weight partitioning, plant height, and cannabinoid content.
- Findings useful for later calibration: spectra changed morphology and cannabinoid profiles; total cannabinoid yield did not differ among the three treatments; flowering time did not differ by the reported R:FR comparison.
- Limitations: limited spectrum treatments, controlled protocol, and no basis for universal spectrum-response constants.
- P1A use: supports keeping spectral/fixture state separate from scalar PPFD, but P1A does not implement spectral or biological response state.

## C. Unresolved questions

- A single universal cannabis PPFD-to-photosynthesis or PPFD-to-yield equation is not established by these studies.
- Cultivar, developmental phase, canopy architecture, CO2, temperature, RH/VPD, root-zone state, and lighting history interact; single-leaf data cannot be silently promoted to whole-canopy response.
- Leaf-temperature measurement or estimation requires a later measurement/energy-balance decision. P1A accepts explicit leaf temperature only.
- A future transpiration model needs sources and calibration that jointly address leaf temperature, VPD, stomatal conductance, airflow, radiation, and root-zone supply.
- Spectrum effects cannot be represented by a single “red versus blue” gameplay scalar without documented spectral measurements and genotype/phase limits.
- Pressure is stored now, but its role in later psychrometric or gas-exchange models remains unresolved.
- Cannabis DLI ranges in individual experiments are treatment conditions, not universal optimal ranges.
- No P1A source justifies a cannabis “ideal VPD,” “ideal temperature,” or flowering trigger. Those remain explicitly unresolved.
