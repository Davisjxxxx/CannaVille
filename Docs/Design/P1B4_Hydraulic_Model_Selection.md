# P1B.4 Hydraulic Model Selection and Architecture

## 1. The Interface: `beta_hydraulic`
Hydraulic stress is communicated to the gas exchange kernel via a dimensionless interface quantity, `beta_hydraulic` (or `stomatal_hydraulic_limitation`), defined on the interval [0.0, 1.0].
- **Semantics:** This is an **INTERFACE** quantity consumed by the stomatal model. It is NOT a universal physical description of root-zone water. Different root-zone architectures compute it differently.
- `beta_hydraulic = 1.0` indicates no hydraulic stomatal limitation.
- `beta_hydraulic → 0.0` indicates increasing modeled hydraulic limitation.
- **Constraints:** Do NOT attach mortality, overall plant health, growth, yield, or tissue-damage semantics to this variable. It dictates short-term stomatal limitation only.

## 2. Substrate Beta Pathway
The required computational pathway for capillary substrate systems (e.g., coir, stonewool) is:
`P1B.3 VWC` → `substrate-specific retention curve` → `substrate matric potential` → `calibrated stress transfer function` → `beta_hydraulic`

## 3. Substrate Hydraulic Profiles
The **van Genuchten** retention model is retained as the preferred mechanism to map VWC to matric potential ($\Psi_{substrate}$). 

**Strict Profile Requirements:**
We explicitly reject the creation of `generic_coco` or `generic_rockwool` production profiles unless supported by appropriate pooled evidence. All substrate profiles must define explicit parameters:
- `profile_id`
- `material/product`
- `composition`
- `theta_r` ($\theta_r$)
- `theta_s` ($\theta_s$)
- `alpha` ($\alpha$)
- `n`
- `m_relationship` (e.g., $m = 1 - 1/n$)
- `fitting_method`
- `sorption_desorption_basis` (must explicitly identify if the curve represents drying/desorption, wetting/sorption, or unknown/combined, as dynamic hysteresis code is deferred)
- `source`
- `valid_range`
- `uncertainty`
- `evidence_classification` (e.g., `REFERENCE_ONLY`, `SYNTHETIC_TEST_VALUE`)

## 4. Beta(psi) Candidate Models
The transfer function `beta_hydraulic(psi_substrate)` maps substrate matric potential to stomatal limitation. Candidate MODEL FORMS (which must be kept separate from CALIBRATION PARAMETERS) include:
1. **Linear Piecewise Water-Potential Function:** Simple linear decline between an onset-of-stress potential and a full-closure potential.
2. **Exponential Function:** Non-linear asymptotic decline as matric potential drops.
3. **Feddes-style Response:** Standard crop-modeling piecewise function encompassing saturation penalty (hypoxia), optimal plateau, and drought decline.
4. **Published Medlyn Moisture-Stress Formulations:** Specialized functions targeting the marginal carbon cost of water based on soil water potential.

No coefficients will be chosen without explicit calibration evidence.

## 5. Medlyn Integration
How `beta_hydraulic` modifies the FvCB-Medlyn gas exchange kernel must be defined.
- **Candidate 1 (Recommended First Implementation):** `g1_effective = g1_reference * beta_hydraulic`
  - *Rationale:* Scaling the Medlyn $g_1$ parameter directly modifies stomatal sensitivity to the marginal carbon cost of water, maintaining mechanistic continuity. 
  - *Classification:* This is documented strictly as an `EMPIRICAL_HYDRAULIC_EXTENSION`, as it is not part of the original Medlyn physiology.
- **Candidate 2:** Multiplying the total stomatal conductance ($g_s$) directly by `beta_hydraulic`.
- **Candidate 3:** Applying a direct maximum $g_s$ cap based on water potential.
- **Candidate 4:** Full hydraulic model tracking SPAC (soil-plant-atmosphere continuum) conductances and xylem cavitation.

**Decision:** Scaling $g_1$ (`Candidate 1`) is selected for P1B.4.1. Non-stomatal drought effects (e.g., modifying FvCB capacity parameters directly) are deferred.

## 6. DWC Correction and Policy
**Correction:** Reservoir storage fraction is NOT physiological hydraulic stress. For example, a reservoir can be 40% full while all active roots remain submerged and have unrestricted access to water. Therefore, directly equating `reservoir_storage_fraction` to hydraulic stress is invalid and must be removed.

**Submerged-Root Fraction:** The true metric for DWC limitation would be the fraction of root mass actively submerged in the nutrient solution. Because this requires explicit root vertical distribution and reservoir geometry—which are not currently available—submerged-root fraction is documented as **FUTURE STATE**. It will not be inferred from reservoir volume fraction and will not be implemented in P1B.4.1.

**DWC Policy for P1B.4.1:**
Do not fabricate a continuous DWC stress curve. The system will use discrete states:
- **Case A:** Root-zone contains sufficient water and no accessibility failure is known. Result: `beta_hydraulic = 1.0`
- **Case B:** P1B.3 reports insufficient root-zone water. Result: preserve explicit shortage state.
- **Case C:** Geometry/contact would be required to determine limitation. Result: `HYDRAULIC_STATE_UNAVAILABLE`

## 7. Final Implementation Recommendation

The exact proposed P1B.4.1 architecture is:

### Substrate Systems
`P1B.3 VWC` → `calibrated retention curve` → `matric potential` → `calibrated beta_hydraulic` → `empirical Medlyn g1 modification`

### DWC Systems
No continuous hydraulic stress curve yet; only discrete unrestricted/shortage/unavailable states as defined above.

**Decision:** **GO**
