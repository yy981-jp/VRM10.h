# Validation Strategy

## 1. Validation layers

VRM10.h uses independent validation layers because no single validator covers every responsibility.

| Layer | What it checks | Authority |
|---|---|---|
| C++ unit tests | Parser behavior, defaults, errors, indexes, regression cases | Our implementation contract |
| Official JSON Schema | JSON structure, types, required fields, enums, array lengths, schema constraints | VRM Consortium schemas |
| VRM asset validator | Full glTF/GLB structure, resource references, and supported VRM extension semantics | Independent implementation |
| VRM10.h cross-reference validation | References against caller-supplied glTF counts/context, Humanoid bone hierarchy, SpringBone chain membership including skipped nodes, center-space restrictions, node-constraint dependency cycles, and mesh/texture/material-mode references | Our public validation API |

A pass in one layer does not imply a pass in another.

## 2. Official VRM schemas

Source:

https://github.com/vrm-c/vrm-specification

CI checks out the VRM specification repository at this pinned revision:

- Repository: vrm-c/vrm-specification
- Revision: 94e82dd346fa6cf0337c4421728640e5252dd38e
- Revision date: 2026-10-02
- Schema family: JSON Schema Draft-04

The VRM schema files reference common glTF schema definitions such as glTFProperty.schema.json and glTFid.schema.json. Those files are maintained in the Khronos glTF repository, so CI checks out that repository separately and copies the two shared schemas into the VRM schema directory before validation.

The glTF schema revision is pinned:

- Repository: KhronosGroup/glTF
- Revision: 5961f897c1cfe51b8e270b2e905d529e2c6127d4

Both pins prevent upstream changes from silently changing CI results. Update pins deliberately and review their schema diffs together.

Automated schema fixtures cover VRMC_vrm, VRMC_springBone, VRMC_node_constraint, VRMC_materials_mtoon, VRMC_springBone_extended_collider, VRMC_springBone_limit, and VRMC_materials_hdr_emissiveMultiplier. The validator accepts an explicit schema filename for the archived HDR extension whose schema filename does not follow the usual .schema.json convention.

The schema job uses jsonschema only in CI. It is not a runtime dependency of VRM10.h.

## 3. Independent VRM asset validator

CI uses:

https://github.com/mrxz/vrm-validator

This project is a fork of Khronos glTF-Validator and documents validation for glTF 2.0 and VRM 1.0. Its CLI emits a JSON report and returns a non-zero exit code when it finds errors.

Pinned source revision:

- Repository: mrxz/vrm-validator
- Revision: 43a0c8df2067d2d592fa18f12b824be8ce8ebb11
- Revision date: 2024-11-13
- License: Apache-2.0
- Build runtime: Dart SDK 2.19.6 (pinned because the validator's pubspec.yaml constrains the SDK to <3.0.0 at this revision)
- Generated CLI: build/bin/vrm_validator

CI builds the validator from source using the upstream documented command:

dart run grinder exe

It runs the generated validator with --all --stdout against tests/fixtures/models/minimal.vrm.gltf. The JSON report is uploaded as a GitHub Actions artifact even when the validation step fails.

The fixture is a deliberately small glTF JSON document containing VRMC_vrm and a minimal humanoid node hierarchy. It tests extension parsing and reference linking without making this project responsible for GLB binary construction. It is not intended to be a renderable avatar or a replacement for real-world .vrm integration fixtures.

### Coverage limitations

The external validator is useful but not exhaustive:

- Its pinned source predates the current VRM specification revision.
- Its registered VRM extension handlers include VRMC_vrm, VRMC_springBone, VRMC_node_constraint, and VRMC_materials_mtoon. The model fixture exercises all four.
- It must not be treated as proof of conformance for VRMC_springBone_extended_collider, VRMC_springBone_limit, or VRMC_materials_hdr_emissiveMultiplier; those are covered by official-schema and direct parser tests.
- It is not the authority for the current official schemas.
- Its Dart runtime and dependency graph are isolated to the external-validator CI job and are not dependencies of this C++ library.

Do not copy validator source into this repository. Use it as an independent CI tool and keep its source revision pinned.

A second historical project, saturday06/VRM-Validator, is a 2019 fork of Khronos glTF-Validator. It is not selected as the primary validator for this VRM 1.0 project.

## 4. CI policy

The required CI jobs are:

1. cpp: configure, compile standalone public-header smoke translation units, and run parser unit tests with GCC and Clang.
2. vrm-schema: validate valid and intentionally invalid fixtures for all seven implemented VRM extension schemas against the pinned official schemas and their pinned shared glTF schemas.
3. external-vrm-validator: build the pinned external validator, validate the minimal VRM-extension fixture, and upload its JSON report.

A job should fail on:
- C++ compilation errors
- Unit test failures
- A valid fixture rejected by the official schema
- An invalid fixture accepted by the official schema
- External-validator errors

External-validator warnings and informational findings should remain visible in logs and reports. They should be reviewed, but should not automatically fail CI unless the project explicitly adopts a particular issue as an error.

## 5. Growing the validation corpus

As the implementation grows:

- Add one or more small JSON fixtures per extension.
- Add invalid fixtures for missing fields, invalid enums, invalid array sizes, and out-of-range values.
- Add model-level .gltf fixtures for cross-reference behavior.
- Supply complete `nodeParents` and `nodeMorphTargetCounts` context arrays in semantic tests when checking node hierarchy, mesh presence, and morph target index validity.
- Add one or more genuine .vrm GLB fixtures when container/resource integration is ready.
- Keep fixtures minimal and document what each one is intended to prove.
- Never rely on a validator-clean model alone; pair it with direct parser tests.
- Keep known validator gaps documented here.

## 6. Updating external pins

When updating the VRM specification, glTF schema source, or external validator:

1. Inspect the upstream changelog and commit diff.
2. Confirm the new revision still contains the referenced schemas or supports the invocation used in CI.
3. Update the pinned revision in .github/workflows/ci.yml.
4. Update the recorded revision and known coverage in this memo.
5. Run the full workflow and review the new validation report.
