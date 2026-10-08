# P1B.4 Scientific Evidence: Hydraulic Stress in Cannabis sativa

## 1. Cannabis Sativa Drought Response
*Cannabis sativa* demonstrates a highly coordinated physiological response to water deficit, primarily mediated through stomatal regulation to prevent excessive decline in leaf water potential ($\Psi_L$) and catastrophic xylem cavitation. The plant actively regulates stomatal conductance in response to water deficit to conserve water and maintain turgor.

### Peer-Reviewed Sources
- Tang et al. (2018). "Photosynthetic response of Cannabis sativa L. to variations in photosynthetic photon flux densities, temperature and CO2 conditions." *Planta*.
- Yep et al. (2020). "Cannabis yield, potency, and leaf photosynthesis respond differently to increasing soil moisture." *Frontiers in Plant Science*.
- Caplan et al. (2017). "Optimal Rate of Organic Fertilizer during the Vegetative-stage for Cannabis Grown in Two Coir-based Substrates." *HortScience*.
- Various studies indicating genotypic variation where some cultivars exhibit isohydric behavior (early stomatal closure) while others act more anisohydric.

## 2. Thresholds for Severe Stress and Permanent Wilting Point (PWP)
The Permanent Wilting Point (PWP) is classically defined as a matric potential of -1.5 MPa (or -15 bar, pF 4.2). At this threshold, water is held so tightly by the soil matrix that roots can no longer extract it, leading to irreversible turgor loss.

However, in controlled environment agriculture using soilless substrates, operational thresholds are often defined via Volumetric Water Content (VWC):
- **Coco Coir:**
  - Saturation occurs around 60-65% VWC.
  - Optimal available water range is typically 35-45% VWC.
  - Severe stress / Operational PWP is generally reached when VWC drops below ~30%. Below this, the remaining water is strongly bound, and localized salt concentrations cause severe osmotic stress.
- **Rockwool:**
  - Has a highly porous, inert structure with a more linear matric potential curve.
  - Can sustain plant water uptake at lower absolute VWCs without visual stress compared to coco, but leaves very little margin for error. Severe stress happens rapidly when the remaining water film breaks.

## 3. Substrate Water Retention Hysteresis
Both coco coir and rockwool exhibit **hysteresis**—their water retention behavior differs depending on whether the medium is drying out (desorption) or being re-wetted (sorption). 
- **Necessity to Model:** For real-time greenhouse control (e.g., steering via micro-irrigation pulses), hysteresis can be significant. However, for a generalized macroscopic simulation of plant growth (like CannaVille), tracking the primary desorption curve is typically sufficient. Explicit hysteresis modeling introduces state-tracking complexity and is recommended as an optional refinement rather than a core requirement for P1B.4.

## 4. Stomatal Conductance ($g_s$) and Assimilation ($A_n$) Feedback
Hydraulic stress in Cannabis limits carbon assimilation ($A_n$) primarily via stomatal closure ($g_s$ reduction), which restricts intercellular CO2 ($C_i$). Direct biochemical damage to the photosynthetic apparatus only occurs under extreme, prolonged stress.
- As matric potential drops, hydraulic signaling and abscisic acid (ABA) accumulation trigger stomatal closure.
- **Modeling Implications:** This necessitates upstream regulation of the stomatal conductance model rather than applying a generic downstream penalty directly to $A_n$.

## 5. Timescale of Recovery After Rewatering
- **Immediate to Hours:** If the stress was mild to moderate (stomatal closure but no significant cavitation or root death), turgor recovery and stomatal reopening typically begin within minutes to hours after rewatering.
- **Days/Irreversible:** If the substrate dried beyond the permanent wilting point, causing root desiccation or severe xylem cavitation, recovery can take days, or the plant may suffer permanent morphological damage and reduced yield potential.
