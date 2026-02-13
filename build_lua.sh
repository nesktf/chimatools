#!/usr/bin/env bash

if ! which fennel > /dev/null; then
  ( >&2 echo "fennel binary not found in PATH")
  exit 1
fi

ROOT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
ROCKSPEC_FILE="${ROOT_DIR}/chimatools-dev-1.rockspec"
OUTPUT_DIR="${ROOT_DIR}/build/lua/chimatools"

mkdir -p "${OUTPUT_DIR}"
find -L "${ROOT_DIR}" -type f -iname '*.fnl' -printf "%P\0" |
  while IFS= read -r -d '' file; do
    name=$(basename ${file})
    echo "Compiling ${ROOT_DIR}/${file}..."
    fennel -c "${ROOT_DIR}/${file}" > "${OUTPUT_DIR}/${name%.*}.lua"
    if [[ $? -ne 0 ]]; then
      echo "Failed to compile ${file}"
      exit 1
    fi
  done

if [[ "${1}" == "luarocks" ]]; then
  if ! which luarocks > /dev/null; then
    ( >&2 echo "luarocks binary not found in PATH")
    exit 1
  fi
  echo "Building rockfile..."
  luarocks --lua-version=5.1 make --local "${ROCKSPEC_FILE}"
fi;
