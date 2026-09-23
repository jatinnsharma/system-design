# Labs — Level 10 — Security, Privacy & Multi-Tenancy

- [ ] implement OIDC auth-code+PKCE login, short-lived access tokens + rotating refresh tokens with reuse detection, and ReBAC document permissions with OpenFGA. Then write a test suite that tries to access another tenant's data 20 different ways.
- [ ] build a pooled multi-tenant API with tenant-scoped tokens, Postgres RLS, per-tenant rate limits, and a cross-tenant leakage test suite. Then design (on paper) the migration to a bridge model for one enterprise tenant.
