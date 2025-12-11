#!/bin/bash

# Shader compilation script for Sapfire Engine
# Compiles HLSL shaders to SPIR-V for Vulkan

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SOURCE_DIR="${SCRIPT_DIR}/assets/shaders"
OUTPUT_DIR="${1:-${SCRIPT_DIR}/build/sapling/assets/shaders}"

echo -e "${GREEN}=== Sapfire Shader Compiler ===${NC}"
echo "Source: ${SOURCE_DIR}"
echo "Output: ${OUTPUT_DIR}"
echo ""

# Create output directory if it doesn't exist
mkdir -p "${OUTPUT_DIR}"

# Check for shader compiler
DXC=""
GLSLC=""

# Try to find dxc (DirectX Shader Compiler with SPIR-V support)
if command -v dxc &> /dev/null; then
    DXC="dxc"
    echo -e "${GREEN}Found dxc${NC}"
elif [ -f "${SCRIPT_DIR}/engine/vendor/dxc/_bin/x64/dxc" ]; then
    DXC="${SCRIPT_DIR}/engine/vendor/dxc/_bin/x64/dxc"
    echo -e "${GREEN}Found dxc (vendor)${NC}"
fi

# Try to find glslc (from Vulkan SDK)
if command -v glslc &> /dev/null; then
    GLSLC="glslc"
    echo -e "${GREEN}Found glslc${NC}"
fi

# Prefer dxc for HLSL, fallback to glslc
if [ -z "$DXC" ] && [ -z "$GLSLC" ]; then
    echo -e "${RED}ERROR: No shader compiler found!${NC}"
    echo "Please install one of:"
    echo "  - DXC (DirectX Shader Compiler): https://github.com/microsoft/DirectXShaderCompiler"
    echo "  - glslc (Vulkan SDK): https://vulkan.lunarg.com/sdk/home"
    exit 1
fi

COMPILER="$DXC"
USE_DXC=1
if [ -z "$COMPILER" ]; then
    COMPILER="$GLSLC"
    USE_DXC=0
    echo -e "${YELLOW}Warning: Using glslc for HLSL (may have compatibility issues)${NC}"
fi

echo "Using compiler: ${COMPILER}"
echo ""

# Compile function
compile_shader() {
    local input_file="$1"
    local shader_model="$2"  # vs_6_0, ps_6_0, cs_6_0
    local entry_point="$3"   # main, VS, PS, CS
    local output_file="$4"
    local extra_flags="$5"  # -D VULKAN

    echo -n "Compiling $(basename "$input_file") [$shader_model/$entry_point]... "

    if [ "$USE_DXC" -eq 1 ]; then
        # DXC with SPIR-V output
        if "$COMPILER" -spirv -T "$shader_model" -E "$entry_point" $extra_flags \
            -Fo "$output_file" "$input_file" 2>&1 | grep -v "^$"; then
            echo -e "${RED}FAILED${NC}"
            return 1
        else
            echo -e "${GREEN}OK${NC}"
            return 0
        fi
    else
        # glslc fallback (for GLSL-style shaders)
        # Note: glslc has limited HLSL support
        if "$COMPILER" -fshader-stage=vertex "$input_file" -o "$output_file" 2>&1 | grep -v "^$"; then
            echo -e "${RED}FAILED${NC}"
            return 1
        else
            echo -e "${GREEN}OK${NC}"
            return 0
        fi
    fi
}

# Compile all HLSL shaders
TOTAL=0
SUCCESS=0
FAILED=0

# Find and compile vertex shaders
for shader in "${SOURCE_DIR}"/*.hlsl; do
    if [ -f "$shader" ]; then
        basename=$(basename "$shader" .hlsl)

        # Check if shader has vertex shader entry point
        if grep -q "VS\s*(" "$shader" || grep -q "main.*VS_OUTPUT" "$shader"; then
            TOTAL=$((TOTAL + 1))
            if compile_shader "$shader" "vs_6_0" "VS" "${OUTPUT_DIR}/${basename}-vulkan.vert.spv" "-D VULKAN -fspv-extension=SPV_EXT_descriptor_indexing  -fspv-target-env=vulkan1.2"; then
                SUCCESS=$((SUCCESS + 1))
            else
                FAILED=$((FAILED + 1))
            fi
        fi

        # Check if shader has pixel/fragment shader entry point
        if grep -q "PS\s*(" "$shader" || grep -q "float4.*main.*:" "$shader"; then
            TOTAL=$((TOTAL + 1))
            if compile_shader "$shader" "ps_6_0" "PS" "${OUTPUT_DIR}/${basename}-vulkan.frag.spv" "-D VULKAN -fspv-extension=SPV_EXT_descriptor_indexing  -fspv-target-env=vulkan1.2"; then
                SUCCESS=$((SUCCESS + 1))
            else
                FAILED=$((FAILED + 1))
            fi
        fi

        # Check if shader has compute shader entry point
        if grep -q "CS\s*(" "$shader" || grep -q "\[numthreads" "$shader"; then
            TOTAL=$((TOTAL + 1))
            if compile_shader "$shader" "cs_6_0" "CS" "${OUTPUT_DIR}/${basename}-vulkan.comp.spv" "-D VULKAN -fspv-extension=SPV_EXT_descriptor_indexing  -fspv-target-env=vulkan1.2"; then
                SUCCESS=$((SUCCESS + 1))
            else
                FAILED=$((FAILED + 1))
            fi
        fi
    fi
done

# Compile standalone GLSL shaders if any
shopt -s nullglob  # Make glob return empty if no matches
for ext in vert frag comp; do
    for shader in "${SOURCE_DIR}"/*."$ext"; do
        if [ -f "$shader" ]; then
            TOTAL=$((TOTAL + 1))
            output="${OUTPUT_DIR}/$(basename "$shader").spv"

            echo -n "Compiling $(basename "$shader")... "
            if [ -n "$GLSLC" ]; then
                if "$GLSLC" "$shader" -o "$output" 2>&1 | grep -v "^$"; then
                    echo -e "${RED}FAILED${NC}"
                    FAILED=$((FAILED + 1))
                else
                    echo -e "${GREEN}OK${NC}"
                    SUCCESS=$((SUCCESS + 1))
                fi
            else
                echo -e "${YELLOW}SKIPPED (no glslc)${NC}"
                TOTAL=$((TOTAL - 1))
            fi
        fi
    done
done
shopt -u nullglob

echo ""
echo "=== Compilation Summary ==="
echo -e "Total:   ${TOTAL}"
echo -e "Success: ${GREEN}${SUCCESS}${NC}"
echo -e "Failed:  ${RED}${FAILED}${NC}"
echo ""

if [ "$FAILED" -gt 0 ]; then
    echo -e "${YELLOW}Warning: Some shaders failed to compile${NC}"
    exit 1
else
    echo -e "${GREEN}All shaders compiled successfully!${NC}"
    exit 0
fi
