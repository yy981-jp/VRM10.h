# VRM10.h Development Policy

## 1. Project purpose

VRM10.h is a standalone C++ library that parses and validates VRM 1.0 extension data.

Scalix is the first intended consumer, but the library must remain useful without Scalix. The public API must not expose Scalix types or require its runtime, renderer, node registry, or build system.

The library is a VRM extension parser, not a .vrm/GLB file loader.

## 2. Scope

### In scope

Implement the VRM 1.0 extensions and closely related extension data needed to consume a VRM 1.0 avatar:

- VRMC_vrm
- VRMC_springBone
- VRMC_node_constraint
- VRMC_materials_mtoon
- VRMC_springBone_extended_collider
- VRMC_springBone_limit
- VRMC_materials_hdr_emissiveMultiplier

The implementation must follow the VRM Consortium specifications and schemas, not the behavior of an existing loader.

### Out of scope

- VRM 0.x compatibility
- VRM Animation (VRMC_vrm_animation / VRMA)
- Loading GLB containers, binary buffers, images, or external resources
- Rendering, animation evaluation, spring simulation, constraints evaluation, or engine-specific scene management
- Providing a complete glTF object model

General glTF extensions may be represented where VRM extension data refers to them, but this library does not become a general glTF parser.

## 3. Language, dependencies, and distribution

- Language baseline: C++17.
- Distribution: header-only.
- Build integration: CMake INTERFACE target, exported as VRM10::VRM10.
- Required dependency: nlohmann::json.
- No required dependency on TinyGLTF, cgltf, fastgltf, a renderer, or an engine.
- MIT license.

The core public API accepts nlohmann::json. A caller extracts the extension object from its chosen glTF loader and passes that JSON value to VRM10.h.

Optional adapters belong in separate headers and must not be included by the core umbrella header. The TinyGLTF adapter is an optional convenience utility only; users may write their own conversion layer.

## 4. Architecture and responsibility boundaries

Keep four concerns separate:

1. JSON parsing converts one extension JSON object into typed C++ data.
2. Structural validation checks required fields, JSON types, array lengths, enum values, and specification-level value constraints.
3. Cross-reference validation checks indexes against caller-supplied glTF counts or context. When supplied, node-parent relationships may also be used for extension-specific hierarchy constraints.
4. Runtime interpretation belongs to the consuming application, not this library.

Extension parsers should be independently usable. A parser receives the JSON value of the extension itself, not a tinygltf::Model or the full glTF document.

Typical extension placement in glTF is:

- VRMC_vrm and VRMC_springBone: root extensions
- VRMC_node_constraint: individual nodes[*].extensions
- VRMC_materials_mtoon and VRMC_materials_hdr_emissiveMultiplier: individual materials[*].extensions
- VRMC_springBone_extended_collider: the location defined by its specification, attached to the corresponding spring-bone collider extension data
- VRMC_springBone_limit: individual SpringBone joint extension data

Do not assume every extension is stored at the glTF root.

## 5. Public data model rules

- Use strongly typed index wrappers (NodeIndex, MaterialIndex, SpringColliderIndex, SpringColliderGroupIndex, ImageIndex, and so on). Do not use bare integers interchangeably.
- File indexes are not engine handles. In particular, NodeIndex must never become a Scalix NodeHandle inside this library.
- Index zero is valid. Absence must be represented explicitly, normally with std::optional.
- Use std::array for closed, specification-defined sets such as humanoid bones.
- Use std::unordered_map<std::string, T> for open, user-defined name sets such as custom expressions.
- Use fixed-size std::array<float, N> for fixed-length vectors when the specification requires exactly N values.
- Preserve specification defaults in the typed model when an optional field is omitted.
- Do not add runtime-derived values or engine-specific cached state to parsed data.
- Do not silently reinterpret unknown enum strings. Return a typed parse error with the offending JSON Pointer.
- Unknown extension objects not implemented by this library are outside this parser's responsibility. Unknown ordinary properties should not be rejected merely because this library does not consume them; schema validation remains the authoritative structural check.

## 6. API and error handling

Public APIs return Result<T> for parse operations. The result must make success and failure unambiguous and provide:

- A stable error code
- A JSON Pointer path to the offending value
- A human-readable message

Do not rely on partially initialized output objects or leave enum fields indeterminate.

Parsing may stop at the first structural error. Validation should be able to collect multiple independent issues in a ValidationReport, each with severity, code, path, and message.

Keep parsing and validation callable independently. A caller may want to inspect parsed data before performing cross-reference checks.

## 7. Specification-first development

The canonical source of truth is the VRM Consortium specification repository:

https://github.com/vrm-c/vrm-specification

For every extension:

1. Read its README and all relevant sub-specification documents.
2. Read the JSON Schema files, including referenced schemas.
3. Record required fields, optional fields, defaults, enum strings, numeric constraints, and reference domains.
4. Define the C++ data model.
5. Implement parser functions.
6. Add positive and negative JSON fixtures.
7. Add cross-reference and semantic tests where applicable.
8. Verify the result against an independent validator when that validator supports the extension.

Do not copy VRM.h's types or parser implementation wholesale. Existing implementations may be consulted for comparison, but the official specification takes precedence.

## 8. Coding conventions

- C++17-compatible standard library only.
- Prefer constexpr std::array lookup tables for fixed enum/string mappings; do not construct hash maps for closed enum sets.
- Prefer small, typed parsing helpers over duplicated JSON access code.
- Use std::string_view for static field and enum names where appropriate.
- Keep parsing helpers in detail; expose only stable public types and functions.
- Include only the headers needed by each public header.
- Keep the umbrella header intentionally small.
- Avoid macros for ordinary field parsing when a function or template is clearer.
- Avoid exceptions for expected malformed-input cases. Exceptions may still arise from allocation failure or programmer misuse of result accessors.
- Keep the core library independent of file I/O and platform APIs.

## 9. Testing and quality

Every feature should have:

- Positive fixtures
- Missing-required-field tests
- Wrong-type tests
- Unknown-enum tests where applicable
- Boundary-value tests for constrained numbers and arrays
- Index-zero tests
- Out-of-range and unresolved-reference tests where applicable
- Regression tests for every discovered bug

CI must combine:
- C++17 compilation and unit tests with GCC and Clang
- Official JSON Schema validation
- Independent VRM asset validation where the external validator supports the feature
- Reviewable machine-readable validation reports

External validators are independent checks, not the sole definition of correctness. A disagreement must be investigated against the official specification.

## 10. Development workflow

- develop is the integration branch.
- Work should be organized into focused commits and feature branches when a change is substantial.
- Keep main suitable for stable releases.
- Public API changes should be deliberate and documented.
- Do not claim a feature is complete until parsing, validation, tests, and specification coverage are addressed.
- Update this memo and memo/VALIDATION.md whenever scope, architecture, or CI policy changes.
