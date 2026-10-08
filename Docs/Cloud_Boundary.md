# Cloud boundary

Cloud implementation is out of scope for this bootstrap. The future cloud layer must surround the simulation rather than replace or duplicate its scientific authority.

## Future responsibilities

- identity and account mapping
- cloud save storage and versioned state migration
- server-time validation for offline elapsed-time reconciliation
- economy and entitlement authority
- telemetry and audit events
- scenario/model-version compatibility checks

## Authority rules

- The authoritative simulation state must be validated server-side for any economy or competitive outcome.
- Client-provided elapsed time is a request, not proof of elapsed time.
- Client-provided plant health, yield, treatment efficacy, or quality is never authoritative.
- Cloud services must record simulation version, scenario version, seed policy, and state schema version where reproducibility matters.
- Identity, save, economy, and telemetry contracts must remain separate from the scientific model implementation.

## Candidate future deployment shape

The engine-independent library may eventually run in a server-side service or trusted worker. The Unreal client can run a local instance for presentation/offline preparation, subject to server reconciliation and product policy. No Google Cloud SDK is included here.
