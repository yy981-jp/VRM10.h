# VRM10.h

A C++17 header-only parser for VRM 1.0 extension data.

The library intentionally does not load .vrm/.glb files and does not depend on a particular glTF loader. It accepts `nlohmann::json` values representing VRM extension objects, then converts them into strongly typed C++ structures.

## Goals

- VRM 1.0 only. VRM 0.x and VRMA are out of scope.
- C++17 and header-only distribution.
- `nlohmann::json` is the only required dependency.
- No dependency on TinyGLTF, cgltf, a renderer, or an engine.
- Parsing and cross-reference validation are separate operations.
- Parsing errors carry an error code and a JSON Pointer path.
- Unknown enum strings are rejected instead of leaving an enum uninitialized.

## Requirements

- C++17 compiler
- CMake 3.15 or newer
- nlohmann/json 3.10 or newer

## Build

```sh
cmake -S . -B build -DVRM10_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

With `VRM10_BUILD_TESTS=ON`, the build also compiles each public header in an isolated translation unit to catch accidental dependence on include order.

## Installation

Install the exported CMake package:

```sh
cmake -S . -B build -DVRM10_BUILD_TESTS=OFF
cmake --build build
cmake --install build --prefix /path/to/prefix
```

A consumer can then use the exported target:

```cmake
find_package(VRM10 CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE VRM10::VRM10)
```

The installed package resolves its required `nlohmann_json` dependency through CMake's `find_dependency`.

## Usage

`parseVrm` receives the value of `extensions.VRMC_vrm`, not the complete glTF document:

```cpp
#include <vrm10/vrm10.hpp>

nlohmann::json vrmExtension = /* extensions["VRMC_vrm"] */;
auto parsed = vrm10::parseVrm(vrmExtension);

if (!parsed) {
	const auto& error = parsed.error();
	// error.code, error.path, error.message
	return;
}

const vrm10::Vrm& vrm = parsed.value();
const auto hips = vrm.humanoid[vrm10::HumanBone::Hips];
```

### TinyGLTF integration (optional)

The core library does not include or require TinyGLTF. If an application uses TinyGLTF, include `vrm10/util/tinygltf.hpp` after making `tiny_gltf.h` available:

```cpp
#include <tiny_gltf.h>
#include <vrm10/util/tinygltf.hpp>
#include <vrm10/vrm10.hpp>

const auto extension = model.extensions.at("VRMC_vrm");
auto parsed = vrm10::parseVrm(vrm10::util::toJson(extension));
```

The adapter converts TinyGLTF's JSON-like `Value` recursively. Binary values are rejected.

### Cross-reference validation

Parsing checks an extension JSON object's structure and field types. Cross-reference validation receives the typed extension data together with the complete glTF JSON document. VRM10.h derives node counts, parent relationships, mesh morph-target counts, material modes, and texture/image counts internally:

```cpp
const auto report = vrm10::validate(vrm, gltfJson);

if (report.hasErrors()) {
	for (const auto& issue : report.issues) {
		// issue.code, issue.path, issue.message
	}
}
```

The public validation overloads are:

- `validate(const Vrm&, const nlohmann::json&)`
- `validate(const SpringBone&, const nlohmann::json&)`
- `validate(const MaterialsMToon&, const nlohmann::json&, std::size_t materialIndex)`
- `validate(const MaterialsEmissiveStrength&, const nlohmann::json&, std::size_t materialIndex)`
- `validate(const std::vector<std::optional<NodeConstraint>>&, const nlohmann::json&)`

Material validators need the index of the glTF material that owns the parsed extension. Node constraints remain node-indexed so dependency cycles can be checked across the complete node graph.

Malformed glTF fields needed by a validation pass are reported through `ValidationReport`; callers do not construct a `ValidationContext`. The validation API checks VRM cross-references and the glTF metadata required for those checks. It is not a replacement for full glTF 2.0 schema validation.

## Current implementation status

- [x] Project skeleton and CMake package target
- [x] Result and diagnostic types
- [x] VRMC_vrm: Meta, Humanoid, FirstPerson, Expressions, LookAt
- [x] Extension-specific cross-reference and semantic validation for VRMC_vrm, VRMC_springBone, VRMC_node_constraint, and VRMC_materials_mtoon
- [x] Optional TinyGLTF-to-JSON utility
- [x] VRMC_springBone
- [x] VRMC_node_constraint
- [x] VRMC_materials_mtoon
- [x] VRMC_springBone_extended_collider
- [x] VRMC_springBone_limit
- [x] VRMC_materials_hdr_emissiveMultiplier (legacy compatibility)
- [x] KHR_materials_emissive_strength
- [x] Positive and negative official JSON Schema fixtures for all seven in-scope VRMC extensions and the Khronos KHR emissive-strength extension
- [x] Real-model parser and cross-reference regression test using the pinned `init.vrm` asset

The SpringBone parser is available independently through `parseSpringBone`:

```cpp
nlohmann::json springBoneExtension = /* extensions["VRMC_springBone"] */;
auto springBone = vrm10::parseSpringBone(springBoneExtension);
if (!springBone) {
	// springBone.error().path and springBone.error().message
	return;
}

const vrm10::SpringBone& data = springBone.value();
```

Cross-reference and semantic validation derives the required metadata from the supplied glTF JSON document:

- Node parent relationships enable Humanoid bone hierarchy checks and SpringBone joint/center-space checks.
- Per-node morph-target counts validate first-person mesh annotations and Expression morph-target bindings.
- Texture, material, and image array sizes validate typed references.
- The selected material's `alphaMode` and `KHR_materials_unlit` extension are used for MToon render-queue and emissive-strength compatibility checks.

Only the glTF fields required by the selected validation pass are inspected. Malformed fields are returned as `ValidationIssue` entries. SpringBone physics and runtime Expression evaluation remain out of scope.

### Other extension parsers

The remaining parsers accept the JSON value of the extension at its glTF-defined location:

- `parseNodeConstraint`: a node's `extensions.VRMC_node_constraint`
- `parseMaterialsMToon`: a material's `extensions.VRMC_materials_mtoon`
- `parseHdrEmissiveMultiplier`: a material's `extensions.VRMC_materials_hdr_emissiveMultiplier` (legacy compatibility)
- `parseMaterialsEmissiveStrength`: a material's `extensions.KHR_materials_emissive_strength`
- `parseSpringBoneExtendedCollider`: standalone parsing of a collider's nested extension
- `parseSpringBoneLimit`: standalone parsing of a joint's nested extension

The SpringBone parser already invokes the last two parsers for `extensions.VRMC_springBone_extended_collider` on each collider and `extensions.VRMC_springBone_limit` on each joint.

Node constraints are validated as a node-indexed array so source references and dependency cycles can be checked together:

```cpp
std::vector<std::optional<vrm10::NodeConstraint>> constraints(nodes.size());
// Parse each node's VRMC_node_constraint and assign it to constraints[i].
const auto report = vrm10::validate(constraints, gltfJson);
```

MToon texture indexes and material-mode-dependent render queue offsets are checked against the complete glTF JSON and the index of the material that owns the extension:

```cpp
const auto report = vrm10::validate(mtoon, gltfJson, materialIndex);
```

The HDR emissive multiplier extension is archived and superseded by `KHR_materials_emissive_strength`. Both parsers are available: the former for compatibility with existing VRM assets, and the latter for current glTF materials. Validation also checks the latter's prohibition on materials using `KHR_materials_unlit`.

For the stated VRM10.h responsibility, the parser, documented semantic checks, official-schema fixtures, and real-model regression test are in place. Semantic validation is intentionally limited to VRM extension rules and the glTF metadata required by those checks; it is not a claim to validate every rule in the complete glTF 2.0 specification.

The implementation is not a general glTF parser, GLB loader, renderer, physics runtime, or constraint evaluator.

## License

MIT. See [LICENSE](LICENSE).

## Development policy

See [memo/DEVELOPMENT.md](memo/DEVELOPMENT.md) for architecture, scope, API, data-model, and coding rules.

## Validation

GitHub Actions runs C++17 tests with GCC and Clang, validates fixtures against the official VRM JSON Schema, and checks both a minimal fixture and the real `init.vrm` release asset with an independent VRM Validator. The real asset is pinned by SHA-256 and is downloaded during CI rather than committed to this repository.

The C++ integration test extracts only the GLB JSON chunk and passes the extension objects plus caller-derived glTF metadata to VRM10.h. It does not add a GLB loader to the library.

See [memo/VALIDATION.md](memo/VALIDATION.md) for pinned upstream revisions, exact coverage, and known limitations.
