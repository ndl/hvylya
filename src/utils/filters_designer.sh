#!/bin/sh
SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
${SCRIPT_DIR}/../../../../carpc.build/hvylya/src/utils/filters-designer ${SCRIPT_DIR}/fm_constants.h.tmpl ${SCRIPT_DIR}/fm_constants.cpp.tmpl ${SCRIPT_DIR}/../hvylya/filters/fm/fm_constants
