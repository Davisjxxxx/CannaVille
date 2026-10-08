# P1B.4 Hydraulic Model Selection and Architecture

## 1. DWC vs. Capillary Substrate Stress Modeling
A unified model must handle both liquid culture (Deep Water Culture - DWC) and capillary substrates (coco coir, rockwool) appropriately:
- **Substrate (Coco/Rockwool):** Water availability is governed by matric potential. As Volumetric Water Content (VWC) decreases, the energy required to extract water increases non-linearly.
- **DWC (Liquid Culture):** Water availability is not limited by matric potential. The roots are submerged directly in a nutrient solution. Stress in DWC is a function of reservoir depletion (fraction of root mass submerged) or oxygen depletion, NOT matric potential.

## 2. Substrate Water-Retention Curve Models
To map VWC to matric potential ($\Psi_m$), a retention model is required.
1. **van Genuchten Model:** Highly accurate, handles the S-shape curve of soils and substrates well. Requires specific parameters ($\alpha, n, m, \theta_r, \theta_s$).
2. **Brooks-Corey Model:** Piecewise continuous and simpler to invert, but less accurate near saturation, which is critical for hydroponic substrates.
3. **Empirical Linear/Piecewise:** Simple thresholding (e.g., stress begins at 40% VWC, reaches 100% at 30% VWC).

**Evaluation:** The **van Genuchten model** is the agricultural standard and is recommended for CannaVille. It maps directly to the physical properties of coco coir and rockwool, allowing realistic dry-back simulation.

## 3. Stomatal Feedback Methods
How should hydraulic stress affect the gas exchange kernel?
1. **Modifying Medlyn $g_1$:** The $g_1$ parameter represents stomatal sensitivity to the marginal carbon cost of water. Scaling $g_1$ downward directly models the physiological response to stress.
2. **Modifying Max $g_s$:** Clamping the maximum allowable stomatal conductance based on water potential.
3. **Generic Empirical Scalar on $A_n$:** Multiplying final net assimilation by an arbitrary stress factor.

**Evaluation:** The empirical scalar on $A_n$ is **REJECTED** because it violates the mechanistic pathway; water stress primarily restricts CO2 diffusion ($g_s$). Scaling the **Medlyn $g_1$** parameter is the recommended approach to maintain the coupled FvCB-Medlyn mechanics.

## 4. Recommended Unified Architecture
**Architecture:** 
- The system computes a unified `WaterAvailabilityFactor` [0.0 to 1.0].
- **For Substrates:** Calculate matric potential using the van Genuchten curve derived from VWC. Map this matric potential to the `WaterAvailabilityFactor` via a stress transfer function.
- **For DWC:** Calculate the `WaterAvailabilityFactor` directly based on the fraction of the root mass submerged in the liquid volume, bypassing matric potential entirely.
- **Integration:** The `WaterAvailabilityFactor` is passed to the gas exchange kernel to scale the Medlyn $g_1$ parameter.

## 5. Model Boundaries
- **Handled Now (P1B.4):** Mapping substrate VWC to matric potential; mapping DWC volume to root availability; computing a stress scalar; applying the scalar to stomatal conductance ($g_1$) in the gas exchange kernel.
- **Deferred to Full Hydraulics:** Explicit xylem cavitation (vulnerability curves), full soil-plant-atmosphere continuum (SPAC) resistance networks, root oxygen stress, hysteresis, and dynamic root growth.

## 6. Hierarchical Calibration Strategy
1. **Level 1 (Direct Measurement):** Substrate porosity/saturation VWC ($\theta_s$), residual water ($\theta_r$).
2. **Level 2 (Literature/Fitted):** van Genuchten parameters ($\alpha, n$) for specific substrates (e.g., standard coco coir vs. rockwool).
3. **Level 3 (Empirical Tuning):** The transfer function mapping matric potential to the Medlyn $g_1$ scaling factor, tuned to match observed Cannabis drought response curves (e.g., stomatal closure starting around 45% VWC, full closure at 30% VWC in coco).

## 7. Rejected Approaches
- **Full Xylem-Cavitation Tracking:** Explicitly tracking vulnerability curves and percent loss of conductance (PLC) throughout the stem is rejected as unnecessarily complex for this phase. It requires extensive parameterization not readily available for diverse Cannabis cultivars.
- **Generic Assimilation Penalty:** As mentioned above, applying a generic penalty directly to $A_n$ is rejected as it breaks the fundamental relationship between transpiration, conductance, and assimilation.

## 8. Necessary State Variables
To support the chosen model, the following variables are required:
- `substrate_type` (Enum: Coco, Rockwool, DWC, Soil)
- **Substrate Parameters:** `van_genuchten_alpha`, `van_genuchten_n`, `theta_r`, `theta_s`
- **Computed State:** `current_matric_potential` (MPa), `water_availability_factor` [0.0 - 1.0]

## 9. Integration with P1B.3 and P1B.1
- **From P1B.3:** The root zone mass balance provides `liquid_water_volume_m3` and `substrate_bulk_volume_m3`, allowing calculation of VWC.
- **To P1B.1:** The computed `water_availability_factor` is passed into `solve_coupled_gas_exchange()` as a stress modifier, which scales the `g1` parameter before solving the FvCB-Medlyn intersection.

## 10. Complexity Cost Estimate
- **Performance Impact:** Negligible. Computing the van Genuchten equation and a scalar modifier involves a few exponentiations per timestep, easily handled within the existing tick budget.
- **State Size Increase:** Minimal. Requires adding ~4-5 parameters to the substrate configuration and 1-2 derived state variables per plant instance. No complex matrices or history buffers are required since hysteresis is deferred.
