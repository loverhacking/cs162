#!/bin/bash

# Comparison script: pwords vs words
# Used to compare the runtime and output content of two programs

# Set color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# File list
FILES=("gutenberg/alice.txt" "gutenberg/metamorphosis.txt" "gutenberg/peter.txt" "gutenberg/sawyer.txt" "gutenberg/time.txt")

# Temporary files
TMP_PWORDS="/tmp/pwords_output.txt"
TMP_WORDS="/tmp/words_output.txt"
TIME_PWORDS="/tmp/pwords_time.txt"
TIME_WORDS="/tmp/words_time.txt"

# Cleanup function
cleanup() {
    rm -f "$TMP_PWORDS" "$TMP_WORDS" "$TIME_PWORDS" "$TIME_WORDS"
}

# Register cleanup function
trap cleanup EXIT

echo -e "${BLUE}Starting comparison of ./pwords and ./words performance and output...${NC}"
echo "=============================================="

# Check if files exist
echo -e "${YELLOW}Checking input files...${NC}"
for file in "${FILES[@]}"; do
    if [ ! -f "$file" ]; then
        echo -e "${RED}Error: File $file does not exist${NC}"
        exit 1
    else
        echo -e "${GREEN}Found: $file${NC}"
    fi
done

echo "=============================================="

# Test ./pwords
echo -e "${YELLOW}Testing ./pwords...${NC}"
if [ ! -f "./pwords" ]; then
    echo -e "${RED}Error: ./pwords does not exist${NC}"
    exit 1
fi

# Use time command to measure time while capturing output
{ time ./pwords "${FILES[@]}" > "$TMP_PWORDS" 2>&1 ; } 2> "$TIME_PWORDS"

PWORDS_EXIT_CODE=$?
if [ $PWORDS_EXIT_CODE -ne 0 ]; then
    echo -e "${RED}./pwords execution failed, exit code: $PWORDS_EXIT_CODE${NC}"
fi

# Test ./words
echo -e "${YELLOW}Testing ./words...${NC}"
if [ ! -f "./words" ]; then
    echo -e "${RED}Error: ./words does not exist${NC}"
    exit 1
fi

{ time ./words "${FILES[@]}" > "$TMP_WORDS" 2>&1 ; } 2> "$TIME_WORDS"

WORDS_EXIT_CODE=$?
if [ $WORDS_EXIT_CODE -ne 0 ]; then
    echo -e "${RED}./words execution failed, exit code: $WORDS_EXIT_CODE${NC}"
fi

echo "=============================================="

# Display time comparison
echo -e "${BLUE}Time comparison:${NC}"
echo -e "${YELLOW}./pwords execution time:${NC}"
cat "$TIME_PWORDS"
echo -e "${YELLOW}./words execution time:${NC}"
cat "$TIME_WORDS"

echo "=============================================="

# Compare output content
echo -e "${BLUE}Output content comparison:${NC}"

# Use diff to compare content
if diff -q -w "$TMP_PWORDS" "$TMP_WORDS" > /dev/null; then
    echo -e "${GREEN}✓ Output content is identical${NC}"
else
    echo -e "${RED}✗ Output content differs${NC}"
    echo -e "${YELLOW}Difference statistics:${NC}"
    diff -u "$TMP_PWORDS" "$TMP_WORDS" | diffstat
    echo ""
    echo -e "${YELLOW}First 10 lines of differences:${NC}"
    diff -u "$TMP_PWORDS" "$TMP_WORDS" | head -20
fi

echo "=============================================="

# Performance analysis
echo -e "${BLUE}Performance analysis:${NC}"

# Extract actual time (from time output)
PWORDS_TIME=$(grep real "$TIME_PWORDS" | awk '{print $2}')
WORDS_TIME=$(grep real "$TIME_WORDS" | awk '{print $2}')

# Convert time to seconds
pwords_seconds=$(echo "$PWORDS_TIME" | awk -F'm|s' '{print $1 * 60 + $2}')
words_seconds=$(echo "$WORDS_TIME" | awk -F'm|s' '{print $1 * 60 + $2}')

# Calculate speed ratio
if command -v bc >/dev/null 2>&1; then
    if (( $(echo "$words_seconds > 0" | bc -l) )); then
        speed_ratio=$(echo "scale=2; $pwords_seconds / $words_seconds" | bc)
        if (( $(echo "$speed_ratio < 1" | bc -l) )); then
            echo -e "./pwords is ${GREEN}$(echo "scale=2; 1/$speed_ratio" | bc)x faster${NC} than ./words"
        else
            echo -e "./words is ${GREEN}$(echo "scale=2; $speed_ratio" | bc)x faster${NC} than ./pwords"
        fi
    fi
else
    echo -e "${YELLOW}Note: 'bc' calculator not available, skipping speed ratio calculation${NC}"
fi

echo "=============================================="
echo -e "${BLUE}Comparison completed${NC}"