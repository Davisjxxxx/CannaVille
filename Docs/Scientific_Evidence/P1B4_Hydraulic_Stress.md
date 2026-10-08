# P1B.4 Scientific Evidence: Hydraulic Stress in Cannabis sativa

## 1. Cannabis Sativa Drought Response
*Cannabis sativa* demonstrates a highly coordinated physiological response to water deficit. This response must be strictly separated into short-term physiological regulation and long-term acclimation. 

**Short-term physiological response:**
- Stomatal closure
- Reduction in net assimilation ($A_n$)
- Reduction in transpiration
- Declining leaf/plant water potential

**Long-term acclimation (Outside P1B.4.1 scope):**
- Leaf-area changes (LAI reduction)
- Stomatal morphology changes
- Induced senescence
- Biomass allocation shifts

P1B.4.1 is strictly concerned with short-term physiological limitation (stomatal regulation). Long-term morphological adjustments belong to future canopy/growth systems.

## 2. Peer-Reviewed Evidence Hierarchy
### Controlled Drought Stress (Caplan et al., 2019)
**Study:** Caplan, Dixon & Zheng (2019). "Increasing Inflorescence Dry Weight and Cannabinoid Content in Medical Cannabis Using Controlled Drought Stress". *HortScience*. DOI: 10.21273/HORTSCI13510-18.
- **Context:** Applied controlled drought stress during the flowering stage.
- **Treatment Criterion:** Used a midday plant water potential of approximately **−1.5 MPa** as the stress threshold. 
- **Important Distinction:** The −1.5 MPa value used in this study is an **experimental treatment criterion**, representing severe stress applied to the plant. It must NOT be described as a universal Cannabis Permanent Wilting Point (PWP), a universal wilting point, a species constant, or a universal substrate threshold. 

### Photosynthetic Response (Tang et al., 2018)
**Study:** Tang et al. (2018). "Photosynthetic response of Cannabis sativa L. to variations in photosynthetic photon flux densities, temperature and CO2 conditions." *Planta*.
- **Context:** Examines gas exchange and environmental responses.
- **Relevance to P1B.4:** Confirms that short-term water shortage is primarily represented through stomatal regulation. It also notes that prolonged water shortage changed canopy traits (e.g., LAI, senescence).
- **Modeling Constraint:** Because canopy traits fall under long-term acclimation, canopy model coefficients from Tang (2018) must NOT be directly transplanted into the current P1B.4 short-term leaf kernel without further compatibility review.

### Other Recent Evidence and Substrate Caution
When reviewing controlled-environment Cannabis drought evidence, strict measurement semantics must be enforced. Specifically:
- **Percent Container Capacity** must NOT be confused with **Volumetric Water Content (VWC)**. A treatment described as "20–30% of container capacity" cannot be rewritten as "20–30% VWC".
- All claims of "saturation thresholds" or "severe stress thresholds" (e.g., historical claims of 60% VWC saturation or 30% VWC severe stress for coir) are removed unless tied to an explicitly identified, measured substrate profile. Universal thresholds are rejected.

## 3. Substrate Hydraulics and Hysteresis
Both stonewool and some soilless organic substrates exhibit physical hysteresis—the water retention curve differs between drying (desorption) and wetting (sorption).
- **Hysteresis Boundary:** Hysteresis is a real phenomenon in substrate physics. However, to isolate system complexity, explicit hysteresis modeling is deferred for the first implementation (P1B.4.1). 
- **Profile Requirement:** Because hysteresis is deferred, every modeled retention curve must explicitly identify whether its formulation represents a drying/desorption curve, a wetting/sorption curve, or an unknown/combined approximation. A non-hysteretic curve cannot be assumed to capture full substrate behavior.

## 4. Parameter Classifications
Every parameter used in the P1B.4 hydraulic subsystem must be classified using the following strict taxonomy. No generic production defaults are permitted.
- `PHYSICAL_CONSTANT`
- `SUBSTRATE_PROFILE`
- `MODEL_PARAMETER`
- `CANNABIS_REFERENCE`
- `CULTIVAR_PROFILE`
- `SYNTHETIC_TEST_VALUE`
- `REQUIRES_CALIBRATION`
