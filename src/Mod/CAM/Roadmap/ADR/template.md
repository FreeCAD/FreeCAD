
# ADR-001: Use PostgreSQL instead of MongoDB

## Status
Accepted

## Context
We need a relational database to support complex joins and ACID transactions. We considered MongoDB for its flexible schema but it lacks transactional guarantees across documents.

## Decision
We'll use PostgreSQL.

## Consequences
- Gains: strong consistency, mature tooling, powerful query engine.
- Costs: more up-front schema design, learning curve for some devs.
