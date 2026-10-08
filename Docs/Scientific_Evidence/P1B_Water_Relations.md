# P1B Water Relations: Scientific Evidence and Literature Review

## B1. Evidence Hierarchy

In evaluating models and parameters for Cannabis sativa water relations, the following evidence hierarchy is rigorously applied:
1.  **Cannabis-specific peer-reviewed primary research:** Direct measurements of gas exchange, transpiration, and water status in controlled environments.
2.  **Controlled-environment crop physiology research:** Mechanistic principles validated on analogous indoor crops (e.g., tomatoes, greenhouse crops).
3.  **Foundational peer-reviewed plant biophysics:** established physical models of energy balance and gas exchange.
4.  **University/government horticultural references:** For established physical definitions (e.g., FAO-56 for reference equations).

Excluded sources: Grow blogs, nutrient company articles, equipment vendors, forums, and unsourced charts.

## B2. Cannabis Evidence Inventory

The following primary studies measure relevant cannabis physiology parameters:

**1. Transpiration-driven mass-balance in Cannabis sativa**
*   **Citation:** Powell, K. and Bauerle, W.L. (2026). "Predicting vegetative phase nutrient uptake in Cannabis sativa L. via transpiration-driven mass-balance." *Frontiers in Plant Science*.
*   **DOI:** 10.3389/fpls.2026.1955837
*   **Cultivar:** "CJ2" and "First Light"
*   **Growth Stage:** Vegetative and Reproductive phases.
*   **Measured Variables:** Transpiration volume, water-use efficiency (WUE), nutrient removal/uptake.
*   **Model-calibration usefulness:** Extremely useful for linking transpiration to root-zone depletion and mass-balance. Highlights cultivar-specific nutrient forecasting.

**2. Stomatal Behavior and Water Use Efficiency**
*   **Citation:** "Water conservation and assimilation is driven by stomatal behaviour in industrial hemp (Cannabis sativa L.)" (2025). *Environmental and Experimental Botany*.
*   **DOI:** 10.1016/j.envexpbot.2025.106119
*   **Measured Variables:** Stomatal conductance ($g_s$), assimilation ($A_n$), recovery from water deficit.
*   **Model-calibration usefulness:** Good for calibrating hydraulic homeostasis and stomatal closure in response to water deficit.

**3. Photosynthetic response to elevated CO2**
*   **Citation:** Chandra, S. et al. (2008). "Photosynthetic response of Cannabis sativa L. to variations in photosynthetic photon flux densities, temperature and CO2 conditions." *Physiology and Molecular Biology of Plants*.
*   **DOI:** 10.1007/s12298-008-0027-x
*   **Conditions:** Elevated CO2 (700 µmol mol⁻¹).
*   **Measured Variables:** Water-use efficiency, transpiration ($E$), stomatal conductance ($g_s$).
*   **Model-calibration usefulness:** Provides data for CO2 dependency in stomatal conductance models (like Medlyn or Ball-Berry).

**4. Stomatal Morphology and Drought Adaptation**
*   **Citation:** "Cannabis sativa genotypes with larger leaf areas have higher potential to adjust stomatal size and density in response to water deficit" (2024). *Plant Stress*.
*   **DOI:** 10.1016/j.stress.2024.100649
*   **Measured Variables:** Stomatal density and size, water deficit stress.
*   **Limitations:** Highly cultivar-specific plasticity. Do not generalize cultivar-specific coefficients.

## B3. Candidate Model Families (Literature Perspective)

### 1. Empirical Cannabis-Specific Transpiration Regressions
Empirical regressions exist linking VPD and PPFD to transpiration in cannabis, but they are often highly cultivar-specific and do not easily generalize outside their calibration bounds. They lack the mechanistic coupling needed to explain *why* transpiration changes with CO2 or root-zone stress.

### 2. Penman-Monteith / Energy-Balance Family
The Penman-Monteith equation is foundational for canopy and leaf-scale energy balance. 
*   **Requirements:** Net radiation, air temperature, VPD, aerodynamic conductance (boundary layer), and canopy/stomatal conductance.
*   **Limitations:** Using FAO-56 reference crop ET is inappropriate for indoor cannabis. Must be applied at the leaf/canopy scale using explicit stomatal and boundary-layer conductance.

### 3. Stomatal-Conductance Models
*   **Ball-Berry Model:** $g_s = g_0 + m \frac{A_n h_s}{c_s}$. Requires coupling to a photosynthesis model (like Farquhar-von Caemmerer-Berry).
*   **Medlyn Model:** $g_s = g_0 + 1.6 \left( 1 + \frac{g_1}{\sqrt{D}} \right) \frac{A_n}{c_a}$. Often preferred for its more mechanistic derivation regarding VPD ($D$). 
*   **Cannabis calibration requirements:** Both require species/cultivar-specific parameters ($m$ or $g_1$) and coupling to assimilation rate ($A_n$), making them computationally and observationally demanding.

### 4. Boundary-Layer Conductance
Boundary-layer conductance ($g_b$) depends on leaf characteristic dimension (size), air velocity, and geometry. In controlled environments, airflow is heterogeneous, making $g_b$ highly variable. A mobile simulation may need to use a simplified empirical approximation based on average room airflow rather than full fluid dynamics.

### 5. Leaf Energy Balance
Estimating leaf temperature requires solving the energy balance equation: Net Radiation = Sensible Heat + Latent Heat.
*   **Comparison:** Solving this iteratively is computationally expensive. Simply accepting measured leaf temperature (when available) or using a simplified offset based on radiation and transpiration may be necessary for performance.

### 6. Root-to-Shoot Water Limitation
Representing hydraulic limitation requires linking substrate water potential to stomatal closure. Mechanistic approaches use xylem vulnerability curves and soil water retention curves, but these are difficult to parameterize for diverse hydroponic substrates and cannabis cultivars.

## B9. Uncertainty Analysis

Important quantities and their evidence status:
*   **VPD effect on Stomatal Conductance:** MODERATELY_SUPPORTED (Evidence exists, but varies by cultivar).
*   **CO2 effect on Stomatal Conductance:** MODERATELY_SUPPORTED.
*   **Stomatal Conductance parameters (e.g., Medlyn g1):** CULTIVAR_SPECIFIC (Requires calibration).
*   **Photosynthetic capacity parameters (Vcmax, Jmax):** CULTIVAR_SPECIFIC.
*   **Boundary layer conductance in indoor canopies:** INSUFFICIENT_EVIDENCE (Highly dependent on specific fan setups and canopy density).
*   **Root-zone hydraulic conductivity / limitation:** REQUIRES_CALIBRATION (Depends entirely on chosen substrate and container geometry).
*   **Physical constants (e.g., latent heat of vaporization):** WELL_SUPPORTED.

## B14. Calibration Evidence: FvCB and Medlyn Parameters

**1. Current CannaVille Calibration Profiles**
All currently implemented profiles (`synthetic_vegetative_test`, `synthetic_high_capacity_test`, `synthetic_flowering_test`) are strictly classified as **SYNTHETIC_TEST_VALUE**.
They are used for numerical solver validation, domain guard testing, and software integration. They must NOT be interpreted as biologically accurate representations of specific Cannabis cultivars until properly sourced, statistically verified, and matched to compatible model formulations.

**2. Solver Formulation Compatibility (Ci vs Cc)**
The current FvCB solver in CannaVille is explicitly **Ci-based** (intercellular CO2). It assumes infinite mesophyll conductance ($g_m$).

**3. Literature Mismatch Warning: Tang et al. (2017) Hemp C3 Photosynthesis**
*   **Study:** Tang, K. et al. (2017). "Hemp (Cannabis sativa L.) leaf photosynthesis in relation to nitrogen content and temperature..."
*   **Issue:** Tang et al. includes mesophyll conductance ($g_m$) and defines parameters on a **Cc-based** (chloroplastic CO2) formulation. Dropping their $V_{cmax}$ directly into a **Ci-based** FvCB model causes a scientific mismatch and parameter distortion.
*   **Resolution:** The previously named `tang2017` profile has been converted to `synthetic_vegetative_test`. Tang 2017 parameters cannot be used as a literature-derived REFERENCE profile in CannaVille unless mesophyll conductance is implemented, or a rigorous Ci-based re-fitting of their raw data is explicitly validated.

**4. Literature Mismatch Warning: Medical-Cannabis (2022) and CBD-Hemp**
*   **Issue:** Generic values like $V_{cmax25}=110$ and $J_{max25}=165$ cannot be retained as a universal "Medical Cannabis" literature-derived profile. Different cultivars and growing environments lead to vastly different photosynthetic capacities.
*   **Resolution:** The previously named `medical2022` and `cbd_hemp` profiles have been converted to `synthetic_high_capacity_test` and `synthetic_flowering_test`. Future literature-derived REFERENCE profiles must preserve: population/cultivar, mean, uncertainty, n, parameter definition, and fitting methodology.

**5. Medlyn Stomatal Conductance Provenance**
*   **Current State:** The parameters $g_0$ and $g_1$ currently in use are **SYNTHETIC_TEST_VALUE** generic angiosperm estimates (~0.01 and ~3.0-4.0).
*   **Future Requirement:** For any profile claiming literature derivation, $g_0$ and $g_1$ must be sourced with documented plant material, growth stage, environmental conditions, fitting methodology, and confidence class. No hidden generic Cannabis $g_1$ is permitted.

**6. Temperature Response Parameters**
*   **Current State:** The activation energies (e.g., `ea_vcmax`, `ea_jmax`), deactivation energies (`hd`), and entropy terms (`sv`) used in the `arrhenius` and `peaked_arrhenius` temperature response functions are currently standard generic C3 parameters (e.g., Bernacchi et al. 2001 or similar). They are classified as **NON_CANNABIS_REFERENCE** / **SYNTHETIC_TEST_VALUE**.
*   **Future Requirement:** To support specific Cannabis cultivars, temperature response curves must be explicitly calibrated from Cannabis-specific gas-exchange data.

## B15. P1B.2 Boundary-Layer Physics

- **Forced Convection**: Boundary layer conductance over a flat plate in laminar flow is calculated using $Sh = 0.66 Re^{1/2} Sc^{1/3}$ (Campbell & Norman 1998, Eq 7.28). This assumes the leaf behaves aerodynamically as a flat plate and that the flow is laminar ($Re < 20,000$).
- **Free Convection**: In near-zero wind conditions, buoyancy-driven free convection becomes significant. Calculated via Grashof number $Gr$, with $Sh = 0.54 (Gr Sc)^{1/4}$ (Campbell & Norman 1998, Eq 7.33).
- **Mixed Convection / Regime Handling**: We calculate both forced and free convection Sherwood numbers and take the maximum to ensure a strictly positive, physically plausible conductance at zero or very low air velocities. 
- **Characteristic Leaf Dimension**: The effective length $d$ of the leaf in the direction of airflow. For P1B.2 this is an explicit model parameter (e.g. 0.05m to 0.10m for typical leaves).
- **Airflow Effects**: Higher air velocity increases conductance. Very large leaves reduce conductance (thicker boundary layer).
- **Transport Coefficients**: 
  - Kinematic viscosity ($\nu$) and vapor diffusivity ($D_v$) are computed dynamically based on air temperature and atmospheric pressure, scaled by $(T/273.15)^{1.75}$ and $(101.325/P)$ according to Campbell & Norman (1998).
- **Assumptions**: Flow is assumed laminar. Turbulence induced by dense canopy geometry is not modeled explicitly in this foundational flat-plate simplification.

## B16. P1B.2 Transpiration Physics

- **Stomatal Conductance**: Sourced from the FvCB + Medlyn coupled solver (P1B.1).
- **Boundary-Layer Conductance**: As detailed above. Both converted to $mol\ m^{-2}\ s^{-1}$.
- **Total Conductance (Series Resistance)**: Treated as a series resistance pathway: $g_{total} = \frac{g_s \cdot g_b}{g_s + g_b}$. This is a simplified per-unit-transpiring-area formulation (assumes one-sided/hypostomatous, or treats $g_s$ as the total equivalent area-weighted conductance). Amphistomatous complexities are deferred.
- **Leaf-Air Vapor Gradient**: The driving gradient is the mole fraction difference of water vapor, $\Delta w = \frac{VPD}{P_{atm}}$, where VPD is the leaf-to-air vapor pressure difference ($e_s(T_{leaf}) - e_a$).
- **Transpiration Flux**: Calculated as $E = g_{total} \times \Delta w$, returning $mol\ m^{-2}\ s^{-1}$.
- **Atmospheric-Pressure Conversion**: Pressure is explicitly passed in kPa to properly convert VPD (kPa) to mole fraction (mol/mol).

## B17. P1B.2 Cannabis Relevance

Cannabis/hemp water-use studies relevant for later validation (not used for foundational leaf boundary constants, which are physics-based):
- **Powell & Bauerle (2026)**: "Predicting vegetative phase nutrient uptake in Cannabis sativa L. via transpiration-driven mass-balance." (Useful for whole-plant aggregation and nutrient validation in future phases).
- **Cannabis empirical ET**: Note that whole-plant empirical water-use values reported in greenhouse studies include canopy aerodynamic resistance (complex $g_b$). These should not be directly inverted to fit a leaf-level $g_b$ without separating $g_s$.
