# Copyright (C) 2017 - 2021 Michael Fabian 'Xaymar' Dirks <info@xaymar.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

# Setup
set(ARCH_INSTR_32 "i386;i686;x86;arm;ARM")
set(ARCH_INSTR_64 "x86_64;AMD64;IA64;arm64;ARM64")
set(ARCH_INSTR_X86 "i386;i686;x86;x86_64;AMD64")
set(ARCH_INSTR_ARM "arm;ARM;arm64;ARM64")
set(ARCH_INSTR_ITANIUM "IA64")
set(ARCH_BITS 0)
set(ARCH_BITS_POINTER 0)
set(ARCH_INST "")

# Bitness
list(FIND ARCH_INSTR_32 "${CMAKE_SYSTEM_PROCESSOR}" FOUND)
if(FOUND GREATER -1)
	set(ARCH_BITS 32)
endif()

list(FIND ARCH_INSTR_64 "${CMAKE_SYSTEM_PROCESSOR}" FOUND)
if(FOUND GREATER -1)
	set(ARCH_BITS 64)
endif()

# Pointer Size (bits)
math(EXPR ARCH_BITS_POINTER "8*${CMAKE_SIZEOF_VOID_P}")

# Basic Instruction Set
list(FIND ARCH_INSTR_X86 "${CMAKE_SYSTEM_PROCESSOR}" FOUND)
if(FOUND GREATER -1)
	set(ARCH_INST "x86")
endif()

list(FIND ARCH_INSTR_ARM "${CMAKE_SYSTEM_PROCESSOR}" FOUND)
if(FOUND GREATER -1)
	set(ARCH_INST "ARM")
endif()

list(FIND ARCH_INSTR_ITANIUM "${CMAKE_SYSTEM_PROCESSOR}" FOUND)
if(FOUND GREATER -1)
	set(ARCH_INST "Itanium")
endif()

message(STATUS "Targetting ${ARCH_INST} with ${ARCH_BITS}bits and a pointer size of ${ARCH_BITS_POINTER}bit.")
