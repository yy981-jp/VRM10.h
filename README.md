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

Parsing checks the extension JSON structure and its field types. It does not know the containing glTF document's node, material, or image counts. Supply those counts separately to validate references:

```cpp
const auto report = vrm10::validate(vrm, {
	model.nodes.size(),
	model.materials.size(),
	model.images.size()
});

if (report.hasErrors()) {
	for (const auto& issue : report.issues) {
		// issue.code, issue.path, issue.message
	}
}
```

## Current implementation status

- [x] Project skeleton and CMake package target
- [x] Result and diagnostic types
- [x] VRMC_vrm: Meta, Humanoid, FirstPerson, Expressions, LookAt
- [x] Basic cross-reference validation for VRMC_vrm and VRMC_springBone
- [x] Optional TinyGLTF-to-JSON utility
- [x] VRMC_springBone
- [x] VRMC_node_constraint
- [x] VRMC_materials_mtoon
- [x] VRMC_springBone_extended_collider
- [x] VRMC_springBone_limit
- [x] VRMC_materials_hdr_emissiveMultiplier
- [x] Positive and negative official JSON Schema fixtures for all seven in-scope extensions
- [ ] Complete semantic validation coverage

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

Cross-reference validation for SpringBone checks node, collider, and collider-group indexes. When `ValidationContext::nodeParents` is supplied with one optional parent per glTF node, it also checks joint ancestry and center-space constraints. It does not evaluate or simulate SpringBone physics.

### Other extension parsers

The remaining parsers accept the JSON value of the extension at its glTF-defined location:

- `parseNodeConstraint`: a node's `extensions.VRMC_node_constraint`
- `parseMaterialsMToon`: a material's `extensions.VRMC_materials_mtoon`
- `parseHdrEmissiveMultiplier`: a material's `extensions.VRMC_materials_hdr_emissiveMultiplier`
- `parseSpringBoneExtendedCollider`: standalone parsing of a collider's nested extension
- `parseSpringBoneLimit`: standalone parsing of a joint's nested extension

The SpringBone parser already invokes the last two parsers for `extensions.VRMC_springBone_extended_collider` on each collider and `extensions.VRMC_springBone_limit` on each joint.

Node constraints are validated as a node-indexed array so source references and dependency cycles can be checked together:

```cpp
std::vector<std::optional<vrm10::NodeConstraint>> constraints(nodes.size());
// Parse each node's VRMC_node_constraint and assign it to constraints[i].
const auto report = vrm10::validate(constraints, nodes.size());
```

MToon texture indexes are checked against the glTF texture array:

```cpp
vrm10::ValidationContext context;
context.textureCount = model.textures.size();
const auto report = vrm10::validate(mtoon, context);
```

The HDR emissive multiplier extension is archived and superseded by `KHR_materials_emissive_strength`; its parser is retained for compatibility with existing VRM assets.

The implementation covers the listed extension data, but it is not a complete glTF parser, GLB loader, renderer, or physics runtime.

## License

MIT. See [LICENSE](LICENSE).

## Development policy

See [memo/DEVELOPMENT.md](memo/DEVELOPMENT.md) for architecture, scope, API, data-model, and coding rules.

## Validation

GitHub Actions runs C++17 tests with GCC and Clang, validates fixtures against the official VRM JSON Schema, and checks a model fixture with an independent VRM Validator. The external validator's JSON report is uploaded as a workflow artifact.

See [memo/VALIDATION.md](memo/VALIDATION.md) for pinned upstream revisions, exact coverage, and known limitations.
