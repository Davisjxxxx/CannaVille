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

**1. Tang et al. (2017) Hemp C3 Photosynthesis**
*   **Study:** Tang, K. et al. (2017). "Hemp (Cannabis sativa L.) leaf photosynthesis in relation to nitrogen content and temperature: implications for hemp as a bio-economically sustainable crop." *GCB Bioenergy*.
*   **Species/Cultivar:** Industrial Hemp (Fiber).
*   **Growth Stage:** Vegetative.
*   **Measurement Temperature:** Varies (evaluated temperature response curves).
*   **Reported Parameters:** $V_{cmax}$ and $J_{max}$ strongly dependent on Specific Leaf Nitrogen (SLN). $V_{cmax25}$ up to ~100 µmol m⁻² s⁻¹ at high N.
*   **Intended CannaVille Use:** Serves as a **REFERENCE** profile for high-capacity vegetative hemp.

**2. Medical-Cannabis A/Ci Parameters (2022)**
*   **Study:** Various medical-cannabis indoor studies (e.g., Zheng et al. / Kelly et al. era 2022).
*   **Species/Cultivar:** Medical *Cannabis sativa* (high-THC).
*   **Growth Stage:** Vegetative and Reproductive.
*   **Reported Parameters:** $V_{cmax25}$ typically ranges from 80 to 120 µmol m⁻² s⁻¹. $J_{max25}$ ranges from 120 to 180 µmol m⁻² s⁻¹.
*   **Intended CannaVille Use:** Serves as a **REFERENCE** profile for high-intensity indoor cultivation (elevated CO2 capable).

**3. Stage-Dependent CBD-Hemp FvCB Parameters**
*   **Species/Cultivar:** CBD Hemp cultivars (e.g., 'BaOx', 'Cherry Wine').
*   **Growth Stage:** Flowering vs Vegetative.
*   **Reported Parameters:** $V_{cmax}$ typically declines during late flowering due to nitrogen remobilization to floral tissues.
*   **Intended CannaVille Use:** Basis for future dynamic readiness profiles. For P1B.1, provides fixed **REFERENCE_FLOWERING** parameters.

**4. Medlyn Stomatal Conductance Model (and Corrigendum)**
*   **Study:** Medlyn, B. E. et al. (2011). "Reconciling the optimal and empirical approaches to modelling stomatal conductance." *Global Change Biology*. (Including Corrigendum).
*   **Model:** $g_s = g_0 + 1.6 \left(1 + \frac{g_1}{\sqrt{D}}\right) \frac{A_n}{c_a}$
*   **Parameters:** $g_0$ (residual conductance, typically ~0.01 mol m⁻² s⁻¹), $g_1$ (slope parameter, plant-type dependent, ~2-5 kPa^0.5 for angiosperms).
*   **Intended CannaVille Use:** Core stomatal conductance algorithm coupling gas exchange ($A_n$) to environment ($D$ and $c_a$).
