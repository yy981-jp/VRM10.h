#!/usr/bin/env python3
"""Validate VRMC_vrm JSON fixtures against the pinned VRM Consortium schema."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

from jsonschema import Draft4Validator, RefResolver


def load_json(path: Path) -> object:
	with path.open("r", encoding="utf-8") as stream:
		return json.load(stream)


def format_error(error: object) -> str:
	path = "/" + "/".join(str(part) for part in error.absolute_path)
	return f"{path}: {error.message}"


def main() -> int:
	parser = argparse.ArgumentParser()
	parser.add_argument("--spec-root", type=Path, required=True)
	parser.add_argument("--fixtures", type=Path, default=Path("tests/fixtures/vrmc_vrm"))
	args = parser.parse_args()

	schema_dir = args.spec_root / "specification" / "VRMC_vrm-1.0" / "schema"
	schema_path = schema_dir / "VRMC_vrm.schema.json"
	schema = load_json(schema_path)

	Draft4Validator.check_schema(schema)
	resolver = RefResolver(
		base_uri=schema_dir.resolve().as_uri() + "/",
		referrer=schema,
	)
	validator = Draft4Validator(schema, resolver=resolver)

	failures = 0
	valid_files = sorted((args.fixtures / "valid").glob("*.json"))
	invalid_files = sorted((args.fixtures / "invalid").glob("*.json"))

	if not valid_files:
		print("No positive schema fixtures were found.", file=sys.stderr)
		return 2
	if not invalid_files:
		print("No negative schema fixtures were found.", file=sys.stderr)
		return 2

	for path in valid_files:
		errors = sorted(
			validator.iter_errors(load_json(path)),
			key=lambda error: (list(map(str, error.absolute_path)), error.message),
		)
		if errors:
			failures += 1
			print(f"FAIL (expected valid): {path}")
			for error in errors:
				print(f"  {format_error(error)}")
		else:
			print(f"PASS (valid): {path}")

	for path in invalid_files:
		errors = sorted(
			validator.iter_errors(load_json(path)),
			key=lambda error: (list(map(str, error.absolute_path)), error.message),
		)
		if errors:
			print(f"PASS (rejected invalid fixture): {path}")
			for error in errors:
				print(f"  {format_error(error)}")
		else:
			failures += 1
			print(f"FAIL (invalid fixture unexpectedly passed): {path}")

	if failures:
		print(f"{failures} schema fixture(s) failed.", file=sys.stderr)
		return 1

	print("All VRMC_vrm schema fixtures passed.")
	return 0


if __name__ == "__main__":
	raise SystemExit(main())
