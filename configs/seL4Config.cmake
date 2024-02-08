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

# helper macro to unify messages printed output
# Usage example: print_message_multiple_options_helper("architectures" aarch32)
macro(print_message_multiple_options_helper str_type default_str)
    message(STATUS "platform ${KernelPlatform} supports multiple ${str_type}, none was given")
    message(STATUS "  defaulting to: ${default_str}")
endmacro()

# This macro is used by platforms to declare which seL4 architecture(s) they
# support. It takes a list and sets up the one selected by KernelSel4Arch. If
# KernelSel4Arch is not set, the architecture specified by the first list
# element is used.
# Usage example: declare_seL4_arch("aarch64" "aarch32")
macro(declare_seL4_arch)
    # Since this is a macro and not a function, ARGV is not a real variable. One
    # must be created to be able iterate over it.
    set(_arch_list "${ARGV}")
    if(NOT KernelSel4Arch)
        # Use first architecture from list as default.
        list(GET _arch_list 0 _default_KernelSel4Arch)
        print_message_multiple_options_helper("architectures" "${_default_KernelSel4Arch}")
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

    # Ideally, the generic way to enable running the kernel as hypervisor would
    # be selecting an architecture and then enabling KernelHypervisorSupport.
    # Practically, the handling on the architectures differs:
    # - x86: KernelVTX
    # - riscv: no hypervisor support yet
    # - arm: KernelArmHypervisorSupport. But for historical reasons (and because
    #        verification still requires this), on ARM/AARCH32 hypervisor
    #        support requires selecting the dedicated architecture 'arm_hyp'. As
    #        a step towards deprecating setting 'arm_hyp' explicitly, this is
    #        activated automatically if the combination KernelSel4ArchAarch32
    #        and KernelArmHypervisorSupport is set.
    # Note that CMake iterates over all scripts multiple times until a stable
    # configuration is reached. Thus we may pass here multiple times. To avoid
    # seeing the same messages multiple times, we just print them when the
    # condition is not met. But we always set the configuration to ensure we are
    # in a well defined state.
    if(KernelSel4ArchAarch32 AND KernelArmHypervisorSupport)
        set(KernelSel4Arch "arm_hyp" CACHE STRING "" FORCE)
        config_set(KernelSel4ArchArmHyp ARCH_ARM_HYP ON)
        message(STATUS "changing KernelSel4ArchAarch32/aarch32 to KernelSel4ArchArmHyp/arm_hyp")
    elseif(KernelSel4ArchArmHyp)
        # Since 'arm_hyp' is a superset of AARCH32, ensure KernelSel4ArchAarch32
        # is enabled, too. Note that config_choice() above has set this to OFF
        # (applies for all other architectures also) explicitly, so we
        # overwrite this again now.
        if(NOT KernelSel4ArchAarch32)
            message(STATUS "KernelSel4ArchArmHyp: enabling KernelSel4ArchAarch32/aarch32 also")
        endif()
        config_set(KernelSel4ArchAarch32 ARCH_AARCH32 ON)
        # Ensure KernelArmHypervisorSupport is set and enabled.
        if(KernelArmHypervisorSupport)
            message(STATUS "KernelSel4ArchArmHyp: enabling KernelArmHypervisorSupport")
        endif()
        set(KernelArmHypervisorSupport ON CACHE BOOL "" FORCE)
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

# Register a platform's config options to be set if it is selected.
# Additionally, the kernel_platforms variable can be used as a record of all
# platforms that can be built once the platform configuration files have been
# processed.
# name: name of the platform, KernelPlatform will be set to this.
# config1: the CMake configuration name. Such as KernelPlatImx7.
# config2: the C header file config name without CONFIG_. Such as PLAT_IMX7_SABRE.
# enable_test: A CMake boolean formula that allows the option to be selected.
#     e.g. "KernelSel4ArchAarch32", or "KernelSel4ArchX86_64 OR KernelSel4ArchIA32"
macro(declare_platform name config1 config2 enable_test)
    list(APPEND kernel_platforms "${name}\;${config1}\;${config2}\;${enable_test}")
    if("${KernelPlatform}" STREQUAL ${name})
        set(${config1} ON CACHE INTERNAL "" FORCE)
        # Write KernelPlatform into the cache in case it is only a local variable
        set(KernelPlatform ${KernelPlatform} CACHE STRING "")
    else()
        set(${config1} OFF CACHE INTERNAL "" FORCE)
    endif()
endmacro()

# helper macro that prints a message that no sub platform is specified and
# the default sub platform will be used
# Usage example: check_platform_and_fallback_to_default(KernelARMPlatform "sabre")
macro(check_platform_and_fallback_to_default var_cmake_kernel_plat default_sub_plat)
    if("${${var_cmake_kernel_plat}}" STREQUAL "")
        print_message_multiple_options_helper("sub platforms" ${default_sub_plat})
        set(${var_cmake_kernel_plat} ${default_sub_plat})
    endif()
endmacro()

# CLK_SHIFT and CLK_MAGIC are generated from tools/reciprocal.py
# based on the TIMER_CLK_HZ to simulate division.
# This could be moved to a cmake function
# in future to build the values on the first build. Note the calculation
# can take a long time though.
macro(declare_default_headers)
    cmake_parse_arguments(
        CONFIGURE
        ""
        "TIMER_FREQUENCY;MAX_IRQ;NUM_PPI;INTERRUPT_CONTROLLER;TIMER;SMMU;CLK_SHIFT;CLK_MAGIC;KERNEL_WCET;TIMER_PRECISION;TIMER_OVERHEAD_TICKS;MAX_SID;MAX_CB"
        ""
        ${ARGN}
    )
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

# Verify that, as a minimum any variables that are used
# to find other build files are actually defined at this
# point. This means at least: KernelArch KernelWordSize

if("${KernelArch}" STREQUAL "")
    message(FATAL_ERROR "Variable 'KernelArch' is not set.")
endif()

if("${KernelWordSize}" STREQUAL "")
    message(FATAL_ERROR "Variable 'KernelWordSize' is not set.")
endif()

config_choice(KernelPlatform PLAT "Select the platform" ${kernel_platforms})

if(KernelArchARM)

    # ToDo: KernelArmArmV is use for two things.
    #
    #       - It's the values passed to "-march=" for the compiler.
    #         This is also use in
    #         - seL4/rumprun/CMakeLists.txt
    #
    #       - It's the subfolder to include.
    #         This is also use in
    #         - seL4_libs/libsel4bench/CMakeLists.txt
    #         - seL4/seL4_tools/elfloader-tool/CMakeLists.txt
    #         But it turns out, that "armv7ve" is just an alias for "armv7-a"
    #         everywhere.

    if(KernelArchArmV7ve)
        set(KernelArchArmV7a ON) # ARMv7-ve is a superset of ARMv7-a.
        set(KernelArmArmV "armv7ve")
    elseif(KernelArchArmV7a)
        set(KernelArmArmV "armv7-a")
    elseif(KernelArchArmV8a)
        set(KernelArmArmV "armv8-a")
    else()
        message(FATAL_ERROR "unsupported ARM architecture")
    endif()
    set(KernelArmArmV "${KernelArmArmV}" CACHE INTERNAL "")

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
    config_set(KernelArchArmV8a ARCH_ARM_V8A "${KernelArchArmV8a}")
    config_set(KernelAArch64SErrorIgnore AARCH64_SERROR_IGNORE "${KernelAArch64SErrorIgnore}")

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
