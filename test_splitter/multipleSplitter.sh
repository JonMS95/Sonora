#!/bin/bash

CUR_DIR=$(dirname $(realpath ${0}))
source ${CUR_DIR}/splitter.sh

multiSplitterPrintUsage()
{
    echo ""
    echo "Usage:"
    echo "  ${0} <input_dir> <output_dir> <output_prefix> <N> <K>"
    echo ""
    echo "Parameters:"
    echo "  input_dir    : directory including audio file to split"
    echo "  output_dir   : directory where chunks will be written"
    echo "  output_prefix: prefix for generated chunk names"
    echo "  N            : window fraction divisor (window = duration / N)"
    echo "  K            : offset fraction divisor (hop = window / K)"
    echo ""
    echo "Example:"
    echo "  ${0} input_songs ./chunks song 5 10"
    echo ""
}

multiSplitter()
{
    local INPUT_DIR="$1"
    local OUT_DIR="$2"
    local OUT_PREFIX="$3"
    local N="$4"
    local K="$5"

    if  [ -z "${INPUT_DIR}" ]   ||  \
        [ -z "${OUT_DIR}" ]     ||  \
        [ -z "${OUT_PREFIX}" ]  ||  \
        [ -z "${N}" ]           ||  \
        [ -z "${K}" ]              ;
        then
            multiSplitterPrintUsage
            return 1
    fi

    for file in  $(ls ${INPUT_DIR});
    do
        splitterSplitAudio ${file} ${OUT_DIR} ${OUT_PREFIX} ${N} ${K}
    done
}

# Execute only when run directly, not when sourced.
if [[ "${BASH_SOURCE[0]}" == "${0}" ]];
then
    multiSplitter "$@"
fi

