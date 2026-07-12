#!/bin/bash

splitterPrintUsage()
{
    echo ""
    echo "Usage:"
    echo "  splitterSplitAudio <input_file> <output_dir> <output_prefix> <N> <K>"
    echo ""
    echo "Parameters:"
    echo "  input_file   : audio file to split"
    echo "  output_dir   : directory where chunks will be written"
    echo "  output_prefix: prefix for generated chunk names"
    echo "  N            : window fraction divisor (window = duration / N)"
    echo "  K            : offset fraction divisor (hop = window / K)"
    echo ""
    echo "Example:"
    echo "  splitterSplitAudio song.wav ./chunks song 5 10"
    echo ""
}

splitterFixFloat()
{
    if [[ "$1" == .* ]]; then
        echo "0$1"
    else
        echo "$1"
    fi
}

splitterSplitAudio()
{
    local INPUT="$1"
    local OUT_DIR="$2"
    local OUT_PREFIX="$3"
    local N="$4"
    local K="$5"

    if  [ -z "$INPUT" ] ||       \
        [ -z "$OUT_DIR" ] ||     \
        [ -z "$OUT_PREFIX" ] ||  \
        [ -z "$N" ] ||           \
        [ -z "$K" ]              ;
        then
            splitterPrintUsage
            return 1
    fi

    mkdir -p "$OUT_DIR"

    local DURATION=$(ffprobe    -v error                                \
                                -show_entries format=duration           \
                                -of default=noprint_wrappers=1:nokey=1  \
                                "$INPUT"                                )

    local WINDOW_RAW
    WINDOW_RAW=$(echo "$DURATION / $N" | bc -l)

    local WINDOW
    WINDOW=$(splitterFixFloat "$WINDOW_RAW")

    local HOP_RAW
    HOP_RAW=$(echo "$WINDOW_RAW / $K" | bc -l)

    local HOP
    HOP=$(splitterFixFloat "$HOP_RAW")

    echo "Input:        ${INPUT}"
    echo "Output dir:   ${OUT_DIR}"
    echo "Duration:     ${DURATION} sec"
    echo "Window:       ${WINDOW} sec (1/$N)"
    echo "Hop:          ${HOP} sec (window/$K)"
    echo ""

    local INDEX=0
    local START="0"

    while (( $(echo "$START + $WINDOW <= $DURATION" | bc -l) ));
    do
        local OUT_BASE=$(basename ${INPUT})
        OUT_BASE=${OUT_BASE%*.wav}
        local OUTPUT="${OUT_DIR}/${OUT_BASE}_${OUT_PREFIX}_$(printf "%03d" "$INDEX").wav"

        ffmpeg -hide_banner -loglevel error \
            -ss "$START"                    \
            -i "$INPUT"                     \
            -t "$WINDOW"                    \
            "$OUTPUT"

        echo "Created: $OUTPUT (start=$START)"

        local NEXT
        NEXT=$(echo "$START + $HOP" | bc -l)

        START=$(splitterFixFloat "$NEXT")

        INDEX=$((INDEX + 1))
    done

    echo ""
    echo "Done. Generated $INDEX chunks."
}

# Execute only when run directly, not when sourced.
if [[ "${BASH_SOURCE[0]}" == "${0}" ]];
then
    splitterSplitAudio "$@"
fi
