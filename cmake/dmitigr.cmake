# -*- cmake -*-
# Copyright (C) Dmitry Igrishin
# For conditions of distribution and use, see file LICENSE.txt

set(dmitigr_source_types
  root_headers
  preprocessed_headers
  headers
  build_only_sources
  implementations
  cmake_sources
  cmake_unpreprocessed
  transunits)

# ------------------------------------------------------------------------------

function(dmitigr_target_compile_options t)
  if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # FIXME: GDB 7.7 doesn't print info of rvalues' that are function arguments
    # properly when compiled with -gdwarf-4. So, using -gdwarf-3 instead.
    target_compile_options(${t} PRIVATE
      -gdwarf-3
      -pedantic
      -Wall
      -Wextra
      -Winline
      -Winit-self
      -Wuninitialized
      -Wmaybe-uninitialized
      -Woverloaded-virtual
      -Wsuggest-override
      -Wlogical-op
      -Wswitch)
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(${t} PRIVATE
      -pedantic
      -Weverything)
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(${t} PRIVATE
      /W4
      /Zc:throwingNew,referenceBinding,noexceptTypes
      /errorReport:none
      /nologo
      /utf-8)
  endif()
endfunction()

# ------------------------------------------------------------------------------

macro(dmitigr_set_library_info lib version_major version_minor description)
  set(dmitigr_${lib}_name "${lib}")
  string(TOUPPER "${lib}" dmitigr_${lib}_NAME)

  string(SUBSTRING "${lib}" 0 1 n)
  string(TOUPPER "${n}" N)
  string(SUBSTRING "${lib}" 1 -1 ame)
  set(dmitigr_${lib}_Name "${N}${ame}")

  set(dmitigr_${lib}_version_major "${version_major}")
  set(dmitigr_${lib}_version_minor "${version_minor}")
  math(EXPR dmitigr_${lib}_version_major_hi "(${version_major} >> 16) & 0x0000ffff" OUTPUT_FORMAT DECIMAL)
  math(EXPR dmitigr_${lib}_version_major_lo "(${version_major}) & 0x0000ffff" OUTPUT_FORMAT DECIMAL)
  math(EXPR dmitigr_${lib}_version_minor_hi "(${version_minor} >> 16) & 0x0000ffff" OUTPUT_FORMAT DECIMAL)
  math(EXPR dmitigr_${lib}_version_minor_lo "(${version_minor}) & 0x0000ffff" OUTPUT_FORMAT DECIMAL)

  set(dmitigr_${lib}_description "${description}")
  set(dmitigr_${lib}_internal_name "dmitigr_${lib}")
  set(dmitigr_${lib}_product_name "Dmitigr ${dmitigr_${lib}_Name}")
endmacro()

macro(dmitigr_propagate_library_settings lib)
  set(dmitigr_${lib}_name ${dmitigr_${lib}_name} PARENT_SCOPE)
  set(dmitigr_${lib}_NAME ${dmitigr_${lib}_NAME} PARENT_SCOPE)
  set(dmitigr_${lib}_Name ${dmitigr_${lib}_Name} PARENT_SCOPE)

  set(dmitigr_${lib}_version_major ${dmitigr_${lib}_version_major} PARENT_SCOPE)
  set(dmitigr_${lib}_version_minor ${dmitigr_${lib}_version_minor} PARENT_SCOPE)
  set(dmitigr_${lib}_version_major_hi ${dmitigr_${lib}_version_major_hi} PARENT_SCOPE)
  set(dmitigr_${lib}_version_major_lo ${dmitigr_${lib}_version_major_lo} PARENT_SCOPE)
  set(dmitigr_${lib}_version_minor_hi ${dmitigr_${lib}_version_minor_hi} PARENT_SCOPE)
  set(dmitigr_${lib}_version_minor_lo ${dmitigr_${lib}_version_minor_lo} PARENT_SCOPE)
  set(dmitigr_${lib}_description ${dmitigr_${lib}_description} PARENT_SCOPE)

  set(dmitigr_${lib}_internal_name ${dmitigr_${lib}_internal_name} PARENT_SCOPE)
  set(dmitigr_${lib}_product_name ${dmitigr_${lib}_product_name} PARENT_SCOPE)

  foreach(st ${dmitigr_source_types})
    set(dmitigr_${lib}_${st} ${dmitigr_${lib}_${st}} PARENT_SCOPE)
  endforeach()

  foreach(suff public private interface)
    set(dmitigr_${lib}_target_link_libraries_${suff} ${dmitigr_${lib}_target_link_libraries_${suff}} PARENT_SCOPE)
    set(dmitigr_${lib}_target_compile_definitions_${suff} ${dmitigr_${lib}_target_compile_definitions_${suff}} PARENT_SCOPE)
    set(dmitigr_${lib}_target_include_directories_${suff} ${dmitigr_${lib}_target_include_directories_${suff}} PARENT_SCOPE)
  endforeach()
endmacro()

# ------------------------------------------------------------------------------

macro(dmitigr_set_library_info_lib_variables lib)
  set(dmitigr_lib_name ${dmitigr_${lib}_name})
  set(dmitigr_lib_NAME ${dmitigr_${lib}_NAME})
  set(dmitigr_lib_Name ${dmitigr_${lib}_Name})
  set(dmitigr_lib_version_major ${dmitigr_${lib}_version_major})
  set(dmitigr_lib_version_minor ${dmitigr_${lib}_version_minor})
  set(dmitigr_lib_version_major_hi ${dmitigr_${lib}_version_major_hi})
  set(dmitigr_lib_version_major_lo ${dmitigr_${lib}_version_major_lo})
  set(dmitigr_lib_version_minor_hi ${dmitigr_${lib}_version_minor_hi})
  set(dmitigr_lib_version_minor_lo ${dmitigr_${lib}_version_minor_lo})
  set(dmitigr_lib_description ${dmitigr_${lib}_description})
  set(dmitigr_lib_internal_name ${dmitigr_${lib}_internal_name})
  set(dmitigr_lib_product_name ${dmitigr_${lib}_product_name})
endmacro()
