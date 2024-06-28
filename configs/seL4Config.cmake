#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: GPL-2.0-only
#

set(KERNEL_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/..")

#
# Architecture selection
#
set(configure_string "")
set(c_sources "")
set(asm_sources "")
set(bf_declarations "")
set(KernelDTSList "")

include(${KERNEL_ROOT_DIR}/tools/internal.cmake)
include(${KERNEL_ROOT_DIR}/tools/helpers.cmake)

# This macro is used by platforms to declare which seL4 architecture(s) they
# support. It takes a list and sets up the one selected by KernelSel4Arch. If
# KernelSel4Arch is not set, the architecture specified by the first list
# element is used.
# Usage example: declare_seL4_arch("aarch64" "aarch32")
macro(declare_seL4_arch)
    # Since this is a macro and not a function, ARGV is not a real variable. We
    # have to create a variable explicitly to be able iterate over it.
    set(_arch_list "${ARGV}")
    if(NOT KernelSel4Arch)
        # Use first architecture from list as default.
        list(GET _arch_list 0 _default_KernelSel4Arch)
        message(STATUS "no specific architecture selected for KernelPlatform '${KernelPlatform}'")
        message(STATUS "  defaulting to: ${_default_KernelSel4Arch}")

        set(KernelSel4Arch "${_default_KernelSel4Arch}" CACHE STRING "" FORCE)
    elseif(NOT "${KernelSel4Arch}" IN_LIST _arch_list)
        message(FATAL_ERROR "KernelSel4Arch '${KernelSel4Arch}' not in '${_arch_list}'")
    endif()

    config_choice(
        KernelSel4Arch
        SEL4_ARCH
        "Architecture mode for building the kernel"
        "aarch32;KernelSel4ArchAarch32;ARCH_AARCH32"
        "aarch64;KernelSel4ArchAarch64;ARCH_AARCH64"
        "arm_hyp;KernelSel4ArchArmHyp;ARCH_ARM_HYP"
        "riscv32;KernelSel4ArchRiscV32;ARCH_RISCV32"
        "riscv64;KernelSel4ArchRiscV64;ARCH_RISCV64"
        "x86_64;KernelSel4ArchX86_64;ARCH_X86_64"
        "ia32;KernelSel4ArchIA32;ARCH_IA32"
    )

    if(KernelSel4ArchArmHyp)
        # arm-hyp is basically aarch32. This should be cleaned up and aligned
        # with other architectures, where hypervisor support is an additional
        # flag. The main blocker here is updating the verification flow.
        config_set(KernelSel4ArchAarch32 ARCH_AARCH32 ON)
    endif()

    config_choice(
        KernelArch
        ARCH
        "Architecture to use when building the kernel"
        "arm;KernelArchARM;ARCH_ARM;KernelSel4ArchAarch32 OR KernelSel4ArchAarch64"
        "riscv;KernelArchRiscV;ARCH_RISCV;KernelSel4ArchRiscV32 OR KernelSel4ArchRiscV64"
        "x86;KernelArchX86;ARCH_X86;KernelSel4ArchX86_64 OR KernelSel4ArchIA32"
    )

    # Set kernel mode options
    if(KernelSel4ArchAarch32 OR KernelSel4ArchRiscV32 OR KernelSel4ArchIA32)
        config_set(KernelWordSize WORD_SIZE 32)
        set(Kernel64 OFF CACHE INTERNAL "")
        set(Kernel32 ON CACHE INTERNAL "")
    elseif(KernelSel4ArchAarch64 OR KernelSel4ArchRiscV64 OR KernelSel4ArchX86_64)
        config_set(KernelWordSize WORD_SIZE 64)
        set(Kernel64 ON CACHE INTERNAL "")
        set(Kernel32 OFF CACHE INTERNAL "")
    else()
        message(FATAL_ERROR "unsupported seL4 architecture: '${KernelSel4Arch}'")
    endif()
endmacro()

# helper function that replaces all '-' by '_' and capitalizes all chars
function(sanitize_str_for_var out_var str)
    string(REPLACE "-" "_" str "${str}")
    string(TOUPPER "${str}" str)
    set(${out_var} "${str}" PARENT_SCOPE)
endfunction()

# Register a platform's config options to be set if it is selected.
# Additionally, the kernel_platforms variable can be used as a record of all
# platforms that can be built once the platform configuration files have been
# processed.
#
# Parameters:
#
#   <name>
#     name of the platform, <KernelPlatform> will be set to this.
#
#   ARCH <arch_list>
#     required, list of architectures, default is first item in list
#
#   MACH <mach>
#     optional, use common SOC architecture

#   CAMKE_VAR <var>
#     optional, customize name of CMake variable for platform, the default name
#     is KernelPlatform_${name} with '-' replaced by '_'
#
#   C_DEFINE <PLAT_xxx>
#     optional, customize name of C code fine for platform, teh default name is
#     PLAT_NAME (name capitalize, '-' replaced by '_'). The C define will be
#     prefixed with "CONFIG_" eventually
#
#   NO_DEFAULT_DTS
#     optional, do not use default DTS. This is "tools/dts/${name}.dts" unless
#     BOARDS is specified, then it is "tools/dts/${board-name}.dts"
#
#   FLAGS <flag> <flag> ...
#     optional, files to add to the build
#
#   SOURCES <file1> <file2> ...
#     optional, files to add to the build
#
#   BOARDS <tupel_list, element=<name>[,<cmake_var>[,<c-define>]]>
#     optional, list of boards, default to first item
function(declare_platform name)

    cmake_parse_arguments(
        PARSE_ARGV
        1
        PARAM
        "NO_DEFAULT_DTS" # options
        "CAMKE_VAR;C_DEFINE;MACH" # one-value keywords
        "ARCH;PLAT_CAMKE_VARS;FLAGS;SOURCES;BOARDS" # multi-value keywords
    )

    if(PARAM_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown arguments to declare_platform: ${PARAM_UNPARSED_ARGUMENTS}")
    endif()

    # Generating a CMake variable from the name requires replacing every "-" by
    # "_". That's all what is needed for the platform names currently in use,
    # new names may require additional sanitizing. We also capitalize all
    # letters to have the C define and the CMake variable aligned closely.
    sanitize_str_for_var(name_as_var "${name}")

    if(NOT DEFINED PARAM_ARCH)
        message(FATAL_ERROR "KernelPlatform '${name}': missing ARCH")
    endif()

    if(NOT DEFINED PARAM_CAMKE_VAR)
        set(PARAM_CAMKE_VAR "KernelPlatform_${name_as_var}")
    endif()

    if(NOT DEFINED PARAM_C_DEFINE)
        set(PARAM_C_DEFINE "PLAT_${name_as_var}")
    endif()

    # disable any CMake variables by default
    set(${PARAM_CAMKE_VAR} OFF CACHE INTERNAL "" FORCE)
    foreach(plat_cmake_var IN LISTS PLAT_CAMKE_VARS PARAM_CAMKE_VAR)
        unset(${plat_cmake_var} CACHE)
        set(${plat_cmake_var} OFF)
    endforeach()

    # Check that PARAM_ARCH contains only valid seL4 architectures. The map
    # below specifies the relation between values the variable KernelSel4Arch
    # can have and the corresponding variable KernelSel4Archxxx.
    set(
        arch_mapping
        "ia32:KernelSel4ArchIA32"
        "x86_64:KernelSel4ArchX86_64"
        "aarch32:KernelSel4ArchAarch32"
        "arm_hyp:KernelSel4ArchArmHyp" # legacy hack, removed one day
        "aarch64:KernelSel4ArchAarch64"
        "riscv32:KernelSel4ArchRiscV32"
        "riscv64:KernelSel4ArchRiscV64"
    )
    # ToDo: don't we have this anywhere already?
    set(list_arch_x86 "ia32;x86_64")
    set(list_arch_arm "aarch64;aarch32;arm_hyp")
    set(list_arch_riscv "riscv64;riscv32")


    set(filter "")
    foreach(a IN LISTS PARAM_ARCH)
        if(NOT ";${arch_mapping};" MATCHES ";${a}:([^;]*);")
            message(FATAL_ERROR "KernelPlatform '${name}': unsupported architecture '${a}'")
        endif()
        list(APPEND filter "${CMAKE_MATCH_1}")
    endforeach()
    string(REPLACE ";" " OR " enable_test "${filter}")

    # now we have the basic platform parameters that the build system needs
    set(
        kernel_platforms
        "${kernel_platforms}"
        "${name},${PARAM_CAMKE_VAR},${PARAM_C_DEFINE},${enable_test}"
        PARENT_SCOPE
    )

    set(board_names "")
    if(DEFINED PARAM_BOARDS)

        foreach(board IN LISTS PARAM_BOARDS)

            string(REPLACE "," ";" board_descr "${board}")
            list(LENGTH board_descr cnt)
            list(GET board_descr 0 board_name)
            list(APPEND board_names "${board_name}")
            sanitize_str_for_var(board_name_as_var "${board_name}")
            set(board_cmake_var "KernelPlatform_${board_name_as_var}")
            set(board_c_define "PLAT_${board_name_as_var}")

            if(cnt GREATER 1)
                list(GET board_descr 1 board_cmake_var)
                if(cnt GREATER 2)
                    list(GET board_descr 2 board_c_define)
                endif()
            endif()

            # disable any CMake variables by default
            unset(${board_cmake_var} CACHE)
            set(${board_cmake_var} OFF)

        endforeach()

        if(name IN_LIST board_names)
            message(FATAL_ERROR "Platform name '${name}' can't be in list of board names")
        endif()

    endif()

    # Assume by default that the platfrom has onl yone board, so the board we
    # selected for the platform is simply the platform name.
    set(selected_board "${name}")
    if(KernelPlatform IN_LIST board_names)
        set(selected_board "${KernelPlatform}")
        # We allow the user to select the target board via KernelPlatform, but
        # in this case we have to change KernelPlatform to hold the platform
        # now to be able to continue.
        set(KernelPlatform "${name}")
    elseif(NOT KernelPlatform STREQUAL "${name}")
        # We are done here if this is not the currently selected platform.
        return()
    elseif(board_names)
        # The KernelPlatform was not set to a specific board name, pick the
        # first one as defualt
        list(GET board_names 0 defaut_board)
        set(selected_board "${defaut_board}")
    endif()

    # If we arrive here, this is the currently selected platform. Do further
    # checks and setup.

    if(NOT KernelSel4Arch)
        message(STATUS "KernelPlatform '${KernelPlatform}': architectures not specified")
        message(STATUS "  options: ${PARAM_ARCH}")
        list(GET PARAM_ARCH 0 KernelSel4Arch)
        message(STATUS "  defaulting to '${KernelSel4Arch}'")
    elseif(NOT KernelSel4Arch IN_LIST PARAM_ARCH)
        message(FATAL_ERROR "KernelPlatform '${KernelPlatform}': unsupported KernelSel4Arch '${KernelSel4Arch}'")
    endif()

    set(KernelPlatform "${KernelPlatform}" CACHE STRING "")
    set(${PARAM_CAMKE_VAR} ON CACHE INTERNAL "" FORCE)
    declare_seL4_arch("${KernelSel4Arch}")

    if(DEFINED PARAM_MACH)
        if(KernelSel4Arch IN_LIST list_arch_arm)
            config_set(KernelArmMach MACH "${PARAM_MACH}")
        else()
            message(FATAL_ERROR "MACH not supported in this architecture")
        endif()
    endif()

    # The usage of KernelARMPlatform/KernelRiscVPlatform is, that a specific
    # board can be selected. We can also hande this via KernelPlatform now, so
    # these variables are deprecated. We still support if for compatibility.
    # Thus, if it is set, check that this is a valid board.
    set(plat_arch_var "")
    if(KernelSel4Arch IN_LIST list_arch_arm)
        set(plat_arch_var_CMAKE "KernelARMPlatform")
        set(plat_arch_var_C "ARM_PLAT")
    elseif(KernelSel4Arch IN_LIST list_arch_riscv)
        set(plat_arch_var_CMAKE "KernelRiscVPlatform")
        set(plat_arch_var_C "RISCV_PLAT")
    elseif(KernelSel4Arch IN_LIST list_arch_x86)
        set(plat_arch_var_C "X86_PLAT")
    else()
        message(FATAL_ERROR "MACH not supported in this architecture")
    endif()
    if(plat_arch_var_CMAKE)
        set(board "${${plat_arch_var_CMAKE}}")
        if(board)
            if((DEFINED PARAM_BOARDS) AND (NOT ${board} IN_LIST board_names))
                message(FATAL_ERROR "unsupported ${plat_arch_var_CMAKE} '${board}'")
            endif()
            # Above we have selected the default board already, this can be
            # overwritten by KernelARMPlatform now.
            message(STATUS "   ${plat_arch_var_CMAKE}: '${board}'")
            if(NOT "${board}" STREQUAL "${selected_board}")
                message(STATUS "   ${plat_arch_var_CMAKE} overwrites default: '${selected_board}'")
            endif()
            set(selected_board "${board}")
        endif()
        config_set(${plat_arch_var_CMAKE} "${plat_arch_var_C}" "${selected_board}")
        # Apply board specific settings
        if(DEFINED PARAM_BOARDS)
            # we know here that PARAM_BOARDS contains ${selected_board}
            # if ${selected_board} is ${name} then use first ${board_name}
            foreach(board IN LISTS PARAM_BOARDS)
                string(REPLACE "," ";" board_descr "${board}")
                list(GET board_descr 0 board_name)
                if("${board_name}" STREQUAL "${selected_board}")
                    sanitize_str_for_var(board_name_as_var "${board_name}")
                    set(board_cmake_var "KernelPlatform_${board_name_as_var}")
                    set(board_c_define "PLAT_${board_name_as_var}")
                    list(LENGTH board_descr cnt)
                    if(cnt GREATER 1)
                        list(GET board_descr 1 board_cmake_var)
                        if(cnt GREATER 2)
                            list(GET board_descr 2 board_c_define)
                        endif()
                    endif()

                    message(STATUS "   set board: '${board_cmake_var}', '${board_c_define}'")
                    config_set("${board_cmake_var}" "${board_c_define}" ON)
                    break()
                endif()
            endforeach()
        endif()
    endif()

    # Provide a generic variable 'KernelBoard' that can be a bit more specific
    # then 'KernelPlatform' in case there are different boards.
    config_set(KernelBoard "${plat_arch_var_C}" "${selected_board}")

    if(DEFINED PARAM_SOURCES)
        # we have to loop here, because add_sources() does not support a list
        # inside a parameter.
        foreach(f IN LISTS PARAM_SOURCES)
            add_sources(CFILES "${f}")
        endforeach()
        set(c_sources "${c_sources}" PARENT_SCOPE)
        set(asm_sources "${asm_sources}" PARENT_SCOPE)
    endif()

    if(DEFINED PARAM_FLAGS)
        foreach(f IN LISTS PARAM_FLAGS)
            set(${f} ON PARENT_SCOPE)
        endforeach()
    endif()

    if(NOT PARAM_NO_DEFAULT_DTS)
        set(main_dts "tools/dts/${selected_board}.dts")
        if(NOT EXISTS "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../${main_dts}")
            message(FATAL_ERROR "missing DTS: ${main_dts}")
        endif()
        set(KernelDTSList "${main_dts}" PARENT_SCOPE)
    endif()

    # ensure the parent sees all the changes that e.g. config_set() made
    set(configure_string "${configure_string}" PARENT_SCOPE)
endfunction()

unset(CONFIGURE_TIMER_FREQUENCY CACHE)
unset(CONFIGURE_MAX_IRQ CACHE)
unset(CONFIGURE_NUM_PPI CACHE)
unset(CONFIGURE_INTERRUPT_CONTROLLER CACHE)
unset(CONFIGURE_TIMER CACHE)
unset(CONFIGURE_SMMU CACHE)
unset(CONFIGURE_CLK_SHIFT CACHE)
unset(CONFIGURE_CLK_MAGIC CACHE)
unset(CONFIGURE_KERNEL_WCET CACHE)
unset(CONFIGURE_TIMER_PRECISION CACHE)
unset(CONFIGURE_TIMER_OVERHEAD_TICKS CACHE)
unset(CONFIGURE_MAX_SID CACHE)
unset(CONFIGURE_MAX_CB CACHE)

# CLK_SHIFT and CLK_MAGIC are generated from tools/reciprocal.py based on the
# TIMER_CLK_HZ to avoid doing a 64-bit division on 32-bit platforms. The
# calculation could happen automatically in a CMake function also, but the
# python script also verifies the multiply-and-shift operation works for every
# 32-bit integer, and this check takes a very long time.
macro(declare_default_headers)
    cmake_parse_arguments(
        CONFIGURE
        ""
        "TIMER_FREQUENCY;MAX_IRQ;NUM_PPI;INTERRUPT_CONTROLLER;TIMER;SMMU;CLK_SHIFT;CLK_MAGIC;KERNEL_WCET;TIMER_PRECISION;TIMER_OVERHEAD_TICKS;MAX_SID;MAX_CB"
        ""
        ${ARGN}
    )
    if(CONFIGURE_UNPARSED_ARGUMENTS)
        message(
            FATAL_ERROR
                "Unknown arguments in declare_default_headers(): ${CONFIGURE_UNPARSED_ARGUMENTS}"
        )
    endif()
    set(CALLED_declare_default_headers 1)
endmacro()

# For all of the common variables we set a default value here if they haven't
# been set by a platform.
foreach(
    var
    IN
    ITEMS
    KernelArmCortexA7
    KernelArmCortexA8
    KernelArmCortexA9
    KernelArmCortexA15
    KernelArmCortexA35
    KernelArmCortexA53
    KernelArmCortexA55
    KernelArmCortexA57
    KernelArmCortexA72
    KernelArchArmV7a
    KernelArchArmV7ve
    KernelArchArmV8a
    KernelAArch64SErrorIgnore
)
    unset(${var} CACHE)
    set(${var} OFF)
endforeach()
unset(KernelArmMach CACHE)
unset(KernelArmMachFeatureModifiers CACHE)
unset(KernelArmCPU CACHE)
unset(KernelArmArmV CACHE)

# Blacklist platforms without MCS support
set(KernelPlatformSupportsMCS ON)

file(GLOB result ${KERNEL_ROOT_DIR}/src/plat/*/config.cmake)
list(SORT result)

foreach(file ${result})
    include("${file}")
endforeach()

config_choice(KernelPlatform PLAT "Select the platform" ${kernel_platforms})

# Verify that a proper minimal configuration has been selected.
foreach(var IN ITEMS KernelPlatform KernelSel4Arch KernelArch KernelWordSize)
    if (NOT ${var})
        message(FATAL_ERROR "Variable '${var}' is not set.")
    endif()
endforeach()

# Now enshrine all the common variables in the config
config_set(KernelArmCortexA7 ARM_CORTEX_A7 "${KernelArmCortexA7}")
config_set(KernelArmCortexA8 ARM_CORTEX_A8 "${KernelArmCortexA8}")
config_set(KernelArmCortexA9 ARM_CORTEX_A9 "${KernelArmCortexA9}")
config_set(KernelArmCortexA15 ARM_CORTEX_A15 "${KernelArmCortexA15}")
config_set(KernelArmCortexA35 ARM_CORTEX_A35 "${KernelArmCortexA35}")
config_set(KernelArmCortexA53 ARM_CORTEX_A53 "${KernelArmCortexA53}")
config_set(KernelArmCortexA55 ARM_CORTEX_A55 "${KernelArmCortexA55}")
config_set(KernelArmCortexA57 ARM_CORTEX_A57 "${KernelArmCortexA57}")
config_set(KernelArmCortexA72 ARM_CORTEX_A72 "${KernelArmCortexA72}")
config_set(KernelArchArmV7a ARCH_ARM_V7A "${KernelArchArmV7a}")
config_set(KernelArchArmV7ve ARCH_ARM_V7VE "${KernelArchArmV7ve}")
config_set(KernelArchArmV8a ARCH_ARM_V8A "${KernelArchArmV8a}")
config_set(KernelAArch64SErrorIgnore AARCH64_SERROR_IGNORE "${KernelAArch64SErrorIgnore}")

# Check for v7ve before v7a as v7ve is a superset and we want to set the
# actual armv to that, but leave armv7a config enabled for anything that
# checks directly against it
if(KernelArchArmV7ve)
    set(KernelArmArmV "armv7ve" CACHE INTERNAL "")
elseif(KernelArchArmV7a)
    set(KernelArmArmV "armv7-a" CACHE INTERNAL "")
elseif(KernelArchArmV8a)
    set(KernelArmArmV "armv8-a" CACHE INTERNAL "")
endif()
if(KernelArmCortexA7)
    set(KernelArmCPU "cortex-a7" CACHE INTERNAL "")
elseif(KernelArmCortexA8)
    set(KernelArmCPU "cortex-a8" CACHE INTERNAL "")
elseif(KernelArmCortexA9)
    set(KernelArmCPU "cortex-a9" CACHE INTERNAL "")
elseif(KernelArmCortexA15)
    set(KernelArmCPU "cortex-a15" CACHE INTERNAL "")
elseif(KernelArmCortexA35)
    set(KernelArmCPU "cortex-a35" CACHE INTERNAL "")
elseif(KernelArmCortexA53)
    set(KernelArmCPU "cortex-a53" CACHE INTERNAL "")
elseif(KernelArmCortexA55)
    set(KernelArmCPU "cortex-a55" CACHE INTERNAL "")
elseif(KernelArmCortexA57)
    set(KernelArmCPU "cortex-a57" CACHE INTERNAL "")
elseif(KernelArmCortexA72)
    set(KernelArmCPU "cortex-a72" CACHE INTERNAL "")
endif()
if(KernelArchARM)
    config_set(KernelArmMach ARM_MACH "${KernelArmMach}")
endif()

if("${TRIPLE}" STREQUAL "")
    set(toolchain_file gcc.cmake)
else()
    set(toolchain_file llvm.cmake)
endif()
set(toolchain_outputfile "${CMAKE_BINARY_DIR}/${toolchain_file}")
if(
    ("${CMAKE_TOOLCHAIN_FILE}" STREQUAL "")
    OR ("${CMAKE_TOOLCHAIN_FILE}" STREQUAL "${toolchain_outputfile}")
)
    if(DEFINED CACHE{CROSS_COMPILER_PREFIX})
        set(cross_prefix $CACHE{CROSS_COMPILER_PREFIX})
    endif()

    configure_file("${KERNEL_ROOT_DIR}/${toolchain_file}" "${toolchain_outputfile}.temp" @ONLY)
    if(EXISTS "${toolchain_outputfile}")
        file(READ "${toolchain_outputfile}.temp" filea)
        file(READ "${toolchain_outputfile}" fileb)
        if(NOT "${filea}" STREQUAL "${fileb}")
            message(
                FATAL_ERROR
                    "Config changes have resulted in a different toolchain file. This is not supported"
            )
        endif()
    endif()
    file(RENAME "${toolchain_outputfile}.temp" "${toolchain_outputfile}")
    set(CMAKE_TOOLCHAIN_FILE "${toolchain_outputfile}" CACHE PATH "")
endif()
