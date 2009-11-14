# ---------------------------------------------------------------------------
# KWWidgets_CREATE_GETTEXT_TARGETS
# This macro can be used to create all the custom commands (and optionally
# targets) necessary to:
# - create a translation template file (pot) out of a set of sources
#   files where translatable strings were marked. This template file
#   will be re-generated each time its sources dependencies are modified.
#   This template file has to be stored out-of-source (say, in the build tree),
#   it is then up to the user to commit it back to the source tree if needed.
# - initialize translation files (po) for a set of locales, out of the 
#   translation template file (pot).
# - merge translation files (po) for a set of locales, out of a new or
#   re-generated translation template file (pot). Translations that were
#   in the po files are kept and new strings to translate found in the
#   pot file are copied to the po file for future translation.
#   Translation files are merged automatically each time the template file
#   is modified, either manually, or when it is re-generated from its sources
#   dependencies.
#   The translation files have to be stored out-of-source (say, in the build 
#   tree), it is then up to the user to commit them back to the source tree
#   if needed.
# - compile the translation files (po) for a set of locates into binary 
#   translation files (mo). Binary files are re-compiled each time the
#   translation file are modified, either manually, or when they have been
#   merge against a modified template file (as a result of manually editing
#   it or modifying its sources dependencies).
#   The binary files are generated from po files that are out-of-source
#   (say, in the build tree). The po files in the source tree are only
#   used to initialize the one in the build tree the first time they
#   are needed.
# - install the mo files.
#
# This macro accepts parameters as arg/value pairs or as a single arg if
# the arg is described as boolean (same as setting the arg to 1). The
# args can be specificied in any order and most of them are optionals.
#
# Required arguments:
# DOMAIN_NAME (string): the translation domain name, i.e. the name of the
#    application or library the translations are created for. 
#    Default to ${PROJECT_NAME} if not found.
# PO_DIR (path): absolute path to the directory where the translation  
#    files (po) are found. 
#    Default to "${CMAKE_CURRENT_SOURCE_DIR}/po" if not found.
# LOCALE_LIST (string): semicolon-separated list of locales to initialize, 
#    merge or compile translation files for (ex: "fr;zh_CN;en").
# MO_BUILD_DIR (path): absolute path to the directory in the *build* tree 
#    where the binary translation files (mo) should be saved.
#    Default "${EXECUTABLE_OUTPUT_PATH}/../locale" if EXECUTABLE_OUTPUT_PATH
#    is found, "${CMAKE_CURRENT_BINARY_DIR}/locale" otherwise.
#
# Optional arguments:
# SOURCES (list): list of source files the translation template file (pot)
#    will be (re)-generated from.
# POT_BUILD_DIR (path): absolute path to the directory in the *build* tree
#    where up-to-date translation template file (pot) should be stored. 
#    Default to "${CMAKE_CURRENT_BINARY_DIR}/po"  if not found.
# PO_BUILD_DIR (path): absolute path to the directory in the *build* tree
#    where up-to-date translation files (po) should be stored. 
#    Default to "${CMAKE_CURRENT_BINARY_DIR}/po"  if not found.
# PO_PREFIX (string): string that will be used to prefix the filename of
#    each translation file.
#    Default to the value of "${DOMAIN_NAME}_"
# MO_INSTALL_DIR (path): directory where the binary translation files (mo)
#    should be installed to.
# COPYRIGHT_HOLDER (string): copyright holder string that will be stored in
#    the translation template file (pot). 
#    Default to the empty string if not found.
# MSGID_BUGS_ADDRESS (string): report address for msgid bugs that will be stored in
#    the translation template file (pot). 
#    Default to the empty string if not found.
# DEFAULT_PO_ENCODING (string): default encoding to be used when initializing
#    new translation file (po) for each locale. This will not change the
#    encoding of existing translation file (po).
#    Default to "utf-8" (Unicode) if not found.
# EXTRA_GETTEXT_KEYWORDS (string): semicolon-separated list of extra keywords
#    that should be recognized as a call to the gettext() function.
# EXTRA_DGETTEXT_KEYWORDS (string): semicolon-separated list of extra keywords
#    that should be recognized as a call to the dgettext() function.
#
# Target arguments:
# By default, custom commands are created to create all the files, as well
# as *one* custom target that can be triggered to refresh all the files.
# This custom target can be added to the 'ALL' target, but is not by default
# as modifying any source file would trigger: re-generating the template
# file (pot), merging all translation files (po), and compiling them into
# binary files (mo).
# TARGET_BASENAME (string): basename of all targets (a suffix is added 
#    depending of each target).
#    Default to ${DOMAIN_NAME} if specified, ${PROJECT_NAME} otherwise.
# CREATE_POT_TARGET (boolean): create a target for the template file (pot),
#    using the '_pot' suffix. 
#    Default to 0.
# CREATE_PO_TARGET (boolean): create one unique target for all translation
#    files (po), using the '_po' suffix. Can be used to refresh all PO files.
#    Depends on the pot file.
#    Default to 0.
# CREATE_PO_LOCALE_TARGETS (boolean): create one target per locale 
#    translation file (po), using the '_po_locale' suffix (say '_po_fr'). Can
#    be used to refresh a single PO file. Depends on the pot file.
#    Default to 0.
# CREATE_MO_TARGET (boolean): create one unique target for all binary
#    translation files (mo), using the '_mo' suffix. Can be used to refresh all
#    MO files (i.e. everything in the translation pipeline). Depends on each
#    single po file. Can be added to the 'ALL' target using 
#    ADD_MO_TARGET_TO_ALL (CREATE_MO_TARGET will therefore be considered true).
#    Default to 1.
# CREATE_MO_LOCALE_TARGETS (boolean): create one target per locale binary
#    translation file (mo), using the '_mo_locale' suffix (say '_mo_fr'). Can
#    be used to refresh a single MO file. Depends on the same po file.
#    Default to 0.
# ADD_MO_TARGET_TO_ALL: add the unique MO target to the 'ALL' target. Doing
#    so is likely to trigger all translation targets each time a source
#    file is modified and compiled. This automatically creates the unique
#    target for all binary translation files (mo), just like if 
#    CREATE_MO_TARGET was true.
#    Default to 0.

cmake_minimum_required(VERSION 2.4)
if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

macro(KWWidgets_CREATE_GETTEXT_TARGETS)

  set(notset_value             "__not_set__")

  # Provide some reasonable defaults

  set(domain_name              ${PROJECT_NAME})
  set(po_dir                   "${CMAKE_CURRENT_SOURCE_DIR}/po")
  set(po_build_dir             "${CMAKE_CURRENT_BINARY_DIR}/po")
  set(pot_build_dir            "${CMAKE_CURRENT_BINARY_DIR}/po")
  set(locale_list              "")
  set(default_po_encoding      "utf-8")
  set(mo_install_dir           "")
  set(copyright_holder         "")
  set(msgid_bugs_address       "foo@bar.com")
  set(sources                  )
  set(po_prefix                ${notset_value})
  set(extra_gettext_keywords   "")
  set(extra_dgettext_keywords  "")

  set(target_basename          ${notset_value})
  set(create_pot_target        0)
  set(create_po_target         0)
  set(create_po_locale_targets 0)
  set(create_mo_target         1)
  set(create_mo_locale_targets 0)
  set(add_mo_target_to_all     0)

  if(EXECUTABLE_OUTPUT_PATH)
    get_filename_component(
      mo_build_dir "${EXECUTABLE_OUTPUT_PATH}/../locale" ABSOLUTE)
  else(EXECUTABLE_OUTPUT_PATH)
    set(mo_build_dir "${CMAKE_CURRENT_BINARY_DIR}/locale")
  endif(EXECUTABLE_OUTPUT_PATH)

  # Parse the arguments

  set(valued_parameter_names "^(TARGET_BASENAME|DOMAIN_NAME|POT_BUILD_DIR|PO_DIR|PO_BUILD_DIR|DEFAULT_PO_ENCODING|MO_BUILD_DIR|MO_INSTALL_DIR|COPYRIGHT_HOLDER|MSGID_BUGS_ADDRESS|PO_PREFIX)$")
  set(boolean_parameter_names "^(ADD_MO_TARGET_TO_ALL|CREATE_POT_TARGET|CREATE_PO_TARGET|CREATE_PO_LOCALE_TARGETS|CREATE_MO_TARGET|CREATE_MO_LOCALE_TARGETS)$")
  set(list_parameter_names "^(SOURCES|LOCALE_LIST|EXTRA_GETTEXT_KEYWORDS|EXTRA_DGETTEXT_KEYWORDS)$")

  set(next_arg_should_be_value 0)
  set(prev_arg_was_boolean 0)
  set(prev_arg_was_list 0)
  set(unknown_parameters)
  
  string(REGEX REPLACE ";;" ";FOREACH_FIX;" parameter_list "${ARGV}")
  foreach(arg ${parameter_list})

    if("${arg}" STREQUAL "FOREACH_FIX")
      set(arg "")
    endif("${arg}" STREQUAL "FOREACH_FIX")

    set(matches_valued 0)
    if("${arg}" MATCHES ${valued_parameter_names})
      set(matches_valued 1)
    endif("${arg}" MATCHES ${valued_parameter_names})

    set(matches_boolean 0)
    if("${arg}" MATCHES ${boolean_parameter_names})
      set(matches_boolean 1)
    endif("${arg}" MATCHES ${boolean_parameter_names})

    set(matches_list 0)
    if("${arg}" MATCHES ${list_parameter_names})
      set(matches_list 1)
    endif("${arg}" MATCHES ${list_parameter_names})
    
    if(matches_valued OR matches_boolean OR matches_list)
      if(prev_arg_was_boolean)
        string(TOLOWER ${prev_arg_name} prev_arg_name)
        set(${prev_arg_name} 1)
      else(prev_arg_was_boolean)
        if(next_arg_should_be_value AND NOT prev_arg_was_list)
          message(FATAL_ERROR 
            "Found ${arg} instead of value for ${prev_arg_name}")
        endif(next_arg_should_be_value AND NOT prev_arg_was_list)
      endif(prev_arg_was_boolean)
      set(next_arg_should_be_value 1)
      set(prev_arg_was_boolean ${matches_boolean})
      set(prev_arg_was_list ${matches_list})
      set(prev_arg_name ${arg})
    else(matches_valued OR matches_boolean OR matches_list)
      if(next_arg_should_be_value)
        if(prev_arg_was_boolean)
          if(NOT "${arg}" STREQUAL "1" AND NOT "${arg}" STREQUAL "0")
            message(FATAL_ERROR 
              "Found ${arg} instead of 0 or 1 for ${prev_arg_name}")
          endif(NOT "${arg}" STREQUAL "1" AND NOT "${arg}" STREQUAL "0")
        endif(prev_arg_was_boolean)
        string(TOLOWER ${prev_arg_name} prev_arg_name)
        if(prev_arg_was_list)
          set(${prev_arg_name} ${${prev_arg_name}} ${arg})
        else(prev_arg_was_list)
          set(${prev_arg_name} ${arg})
          set(next_arg_should_be_value 0)
        endif(prev_arg_was_list)
      else(next_arg_should_be_value)
        set(unknown_parameters ${unknown_parameters} ${arg})
      endif(next_arg_should_be_value)
      set(prev_arg_was_boolean 0)
    endif(matches_valued OR matches_boolean OR matches_list)

  endforeach(arg)

  if(next_arg_should_be_value)
    if(prev_arg_was_boolean)
      string(TOLOWER ${prev_arg_name} prev_arg_name)
      set(${prev_arg_name} 1)
    else(prev_arg_was_boolean)
      if(prev_arg_was_list)
        string(TOLOWER ${prev_arg_name} prev_arg_name)
        set(${prev_arg_name} ${${prev_arg_name}} ${arg})
      else(prev_arg_was_list)
        message(FATAL_ERROR "Missing value for ${prev_arg_name}")
      endif(prev_arg_was_list)
    endif(prev_arg_was_boolean)
  endif(next_arg_should_be_value)
  if(unknown_parameters)
    message(FATAL_ERROR "Unknown parameter(s): ${unknown_parameters}")
  endif(unknown_parameters)

  # Fix some defaults

  if(${target_basename} STREQUAL ${notset_value})
    set(target_basename ${domain_name})
  endif(${target_basename} STREQUAL ${notset_value})

  if(${po_prefix} STREQUAL ${notset_value})
    set(po_prefix "${domain_name}_")
  endif(${po_prefix} STREQUAL ${notset_value})

  # Create the targets

  if(NOT "${sources}" STREQUAL "")
    kwwidgets_create_pot_target(
      "${domain_name}"
      "${pot_build_dir}"
      "${po_dir}"
      "${copyright_holder}"
      "${msgid_bugs_address}"
      "${sources}"
      "${target_basename}"
      "${create_pot_target}"
      "${extra_gettext_keywords}"
      "${extra_dgettext_keywords}"
      )
  endif(NOT "${sources}" STREQUAL "")
  
  kwwidgets_create_po_targets(
    "${domain_name}"
    "${pot_build_dir}"
    "${po_dir}"
    "${po_build_dir}"
    "${po_prefix}"
    "${locale_list}"
    "${default_po_encoding}"
    "${target_basename}"
    "${create_po_target}"
    "${create_po_locale_targets}"
    )

  kwwidgets_create_mo_targets(
    "${domain_name}"
    "${po_dir}"
    "${po_build_dir}"
    "${po_prefix}"
    "${locale_list}"
    "${mo_build_dir}"
    "${mo_install_dir}"
    "${target_basename}"
    "${create_mo_target}"
    "${create_mo_locale_targets}"
    "${add_mo_target_to_all}"
    )

endmacro(KWWidgets_CREATE_GETTEXT_TARGETS)

# ---------------------------------------------------------------------------
# KWWidgets_GET_POT_FILENAME
# Given a translation domain and the location of a directory, return the
# filename to the domain template file (pot).
# 'varname': name of the var the template filename should be stored into
# 'domain_name': translation domain name (i.e. name of application or library)
# 'pot_build_dir': path in the build tree where the template should be stored

macro(KWWidgets_GET_POT_FILENAME varname domain_name pot_build_dir)

  set(${varname} "${pot_build_dir}/${domain_name}.pot")

endmacro(KWWidgets_GET_POT_FILENAME)

# ---------------------------------------------------------------------------
# KWWidgets_GET_PO_FILENAME
# Given a PO directory, a prefix and a locale, return the filename to the
# translation file (po) for that locale.
# 'varname': name of the var the translation filename should be stored into
# 'po_dir': path to the po directory where the PO file are stored
# 'po_prefix': string that is used to prefix each translation file.
# 'locale': a locale (say, "fr")

macro(KWWidgets_GET_PO_FILENAME varname po_dir po_prefix locale)

  set(${varname} "${po_dir}/${po_prefix}${locale}.po")

endmacro(KWWidgets_GET_PO_FILENAME)

# ---------------------------------------------------------------------------
# KWWidgets_GET_PO_SAFE_BUILD_DIR
# Given a PO directory, a PO build directory, returns either the
# PO build directory if it is different than the PO directory, or
# a directory in the build tree. This macro is used to get a safe place
# to write PO related files
# 'varname': name of the var the PO safe build dir should be stored into
# 'po_dir': path to the po directory where the PO file are stored
# 'po_build_dir': build path where up-to-date PO files should be stored

macro(KWWidgets_GET_PO_SAFE_BUILD_DIR varname po_dir po_build_dir)

  if("${po_build_dir}" STREQUAL "${po_dir}")
    set(${varname} "${CMAKE_CURRENT_BINARY_DIR}/po")
    #SET_DIRECTORY_PROPERTIES(PROPERTIES CLEAN_NO_CUSTOM 1)
  else("${po_build_dir}" STREQUAL "${po_dir}")
    set(${varname} "${po_build_dir}")
  endif("${po_build_dir}" STREQUAL "${po_dir}")

endmacro(KWWidgets_GET_PO_SAFE_BUILD_DIR)

# ---------------------------------------------------------------------------
# KWWidgets_GET_MO_FILENAME
# Given a translation domain, a MO build directory, and a locale, return the
# filename to the binary translation file (mo) for that locale and domain.
# 'varname': name of the var the translation filename should be stored into
# 'domain_name': translation domain name (i.e. name of application or library)
# 'mo_build_dir': directory where the binary MO files should be saved to
# 'locale': a locale (say, "fr")

macro(KWWidgets_GET_MO_FILENAME varname domain_name mo_build_dir locale)

  set(${varname} "${mo_build_dir}/${locale}/LC_MESSAGES/${domain_name}.mo")

endmacro(KWWidgets_GET_MO_FILENAME)

# ---------------------------------------------------------------------------
# KWWidgets_GET_ABSOLUTE_SOURCES
# Given a list of sources, return the corresponding absolute paths
# 'varname': name of the var the list of absolute paths should be stored into
# 'sources': list of source files

macro(KWWidgets_GET_ABSOLUTE_SOURCES varname sources)

  set(${varname})
  foreach(file ${sources})
    get_filename_component(abs_file ${file} ABSOLUTE)
    if(NOT EXISTS ${abs_file})
      set(abs_file "${CMAKE_CURRENT_SOURCE_DIR}/${file}")
    endif(NOT EXISTS ${abs_file})
    set(${varname} ${${varname}} ${abs_file})
  endforeach(file)

endmacro(KWWidgets_GET_ABSOLUTE_SOURCES)

# ---------------------------------------------------------------------------
# KWWidgets_GET_RELATIVE_SOURCES
# Given a list of sources, return the corresponding relative paths to
# a directory.
# 'varname': name of the var the list of absolute paths should be stored into
# 'dir': path to the dir we want relative path from
# 'sources': list of *absolute* path to the source files

macro(KWWidgets_GET_RELATIVE_SOURCES varname dir sources)

  get_filename_component(dir_abs ${dir} ABSOLUTE)

  set(${varname})
  foreach(file ${sources})
    file(RELATIVE_PATH rel_file "${dir}" "${file}")
    set(${varname} ${${varname}} ${rel_file})
  endforeach(file)

endmacro(KWWidgets_GET_RELATIVE_SOURCES)

# ---------------------------------------------------------------------------
# KWWidgets_CREATE_POT_TARGET
# Given a domain name, the location of a PO directory, and a list of sources,
# create a custom command/target to generate a translation template file (pot)
# from the source files.
# 'domain_name': translation domain name (i.e. name of application or library)
# 'pot_build_dir': path in the build tree where the template should be stored
# 'po_dir': path to the po directory where the original PO files are found
# 'copyright_holder': optional copyright holder of the template file
# 'msgid_bugs_address': report address for msgid bugs in the template file
# 'sources': list of source files the template file will be generated from
# 'target_basename': basename of the template file target
# 'create_pot_target': if true, create pot target (on top of the command)
# 'extra_gettext_keywords': semicolon-separated list of extra gettext keywords
# 'extra_dgettext_keywords':semicolon-separated list of extra dgettext keywords

macro(KWWidgets_CREATE_POT_TARGET
    domain_name
    pot_build_dir
    po_dir
    copyright_holder
    msgid_bugs_address
    sources
    target_basename 
    create_pot_target
    extra_gettext_keywords
    extra_dgettext_keywords
    )

  kwwidgets_get_pot_filename(pot_build_file 
    "${domain_name}" "${pot_build_dir}")

  # We need the absolute path to each source file

  kwwidgets_get_absolute_sources(abs_sources "${sources}")

  # Put the list on sources to internationalize in an internal cache var
  # so that sub-projects can use it to create their own translation

  set(${domain_name}_INTERNATIONALIZED_SRCS_INTERNAL "${abs_sources}"
    CACHE INTERNAL "Sources that were internationalized for ${domain_name}")

  # Get relative sources to the PO files

  kwwidgets_get_relative_sources(rel_sources "${po_dir}" "${abs_sources}")

  # The extra keywords

  set(keywords)
  foreach(keyword ${extra_gettext_keywords})
    set(keywords ${keywords} 
      "--keyword=${keyword}" "--flag=${keyword}:1:pass-c-format")
  endforeach(keyword)
  foreach(keyword ${extra_dgettext_keywords})
    set(keywords ${keywords} 
      "--keyword=${keyword}:2" "--flag=${keyword}:2:pass-c-format")
  endforeach(keyword)

  # Define a symbol in each source file that can be used by dgettext

  set_source_files_properties(${sources}
    COMPILE_FLAGS "-DGETTEXT_DOMAIN=\\\"${domain_name}\\\"")  

  file(MAKE_DIRECTORY ${pot_build_dir})

  # Output the list of sources to a file. This fill will be read
  # by xgettext (so that we do not have to pass it as a huge command
  # line argument below)

  kwwidgets_get_po_safe_build_dir(
    safe_build_dir "${po_dir}" "${pot_build_dir}")

  set(files_from "${safe_build_dir}/${domain_name}_srcs.txt")

  string(REGEX REPLACE ";" "\n" contents "${rel_sources}")
  file(WRITE "${files_from}" "${contents}")
  #CONFIGURE_FILE(${KWWidgets_TEMPLATES_DIR}/KWWidgetsContents.in ${files_from})

  # We need a dummy file that will just say: this POT target is up to date as
  # far as its dependencies are concerned. This will prevent the POT
  # target to be triggered again and again because the sources are older
  # than the POT, but the POT does not really need to be changed, etc.

  kwwidgets_get_pot_filename(pot_uptodate_file 
    "${domain_name}" "${safe_build_dir}")
  set(pot_uptodate_file "${pot_uptodate_file}.upd")

  # Extract strings to translate to template file (pot)

  if(NOT "${GETTEXT_XGETTEXT_EXECUTABLE}" STREQUAL "")
    set(options "--foreign-user")
    set(keywords ${keyword}
      "--keyword=_" "--flag=_:1:pass-c-format"
      "--keyword=N_" "--flag=N_:1:pass-c-format"
      "--flag=autosprintf:1:c-format"
      "--keyword=kww_sgettext" "--flag=kww_sgettext:1:pass-c-format"
      "--keyword=kww_sdgettext:2" "--flag=kww_sdgettext:2:pass-c-format"
      "--keyword=k_" "--flag=k_:1:pass-c-format"
      "--keyword=ks_" "--flag=ks_:1:pass-c-format"
      "--keyword=s_" "--flag=s_:1:pass-c-format"
      "--flag=kww_printf:1:c-format" 
      "--flag=kww_sprintf:2:c-format" 
      "--flag=kww_fprintf:2:c-format")

    add_custom_command(
      OUTPUT "${pot_uptodate_file}"
      DEPENDS ${abs_sources}
      COMMAND ${CMAKE_COMMAND} 
      ARGS -E chdir "${po_dir}" ${CMAKE_COMMAND} -DCMAKE_BACKWARDS_COMPATIBILITY:STRING=${CMAKE_BACKWARDS_COMPATIBILITY} -Dpot_build_file:STRING=${pot_build_file} -Dpot_uptodate_file:STRING=${pot_uptodate_file} -Dpo_dir:STRING=${po_dir} -Doptions:STRING="${options}" -Dkeywords:STRING="${keywords}" -Dcopyright_holder:STRING="${copyright_holder}" -Dmsgid_bugs_address:STRING="${msgid_bugs_address}" -Dfiles_from:STRING=${files_from} -DGETTEXT_XGETTEXT_EXECUTABLE:STRING=${GETTEXT_XGETTEXT_EXECUTABLE} -P "${KWWidgets_CMAKE_DIR}/KWWidgetsGettextExtract.cmake")
    if(create_pot_target)
      add_custom_target(${target_basename}_pot DEPENDS ${pot_build_file})
    endif(create_pot_target)
  endif(NOT "${GETTEXT_XGETTEXT_EXECUTABLE}" STREQUAL "")

endmacro(KWWidgets_CREATE_POT_TARGET)

# ---------------------------------------------------------------------------
# KWWidgets_CREATE_PO_TARGETS
# Given a domain name, the location of a PO build directory, and a list of
# locales create multiple custom commands/targets to initialize and/or merge 
# the translation files (po) for each locale. Each translation file 
# 'po_build_dir'/locale.po (say, 'po_build_dir'/fr.po) is either initialized
# from or merged against the translation template file in the 'pot_build_dir' 
# directory for the same domain (say, 'pot_build_dir'/'domain_name'.pot). 
# The default encoding of each newly initialized PO file can be specified too.
# 'domain_name': translation domain name (i.e. name of application or library)
# 'pot_build_dir': path in the build tree where the template should be stored
# 'po_dir': path to where the original PO file are found
# 'po_build_dir': build path where up-to-date PO files should be stored
# 'po_prefix': string that will be used to prefix each translation file.
# 'locale_list': semicolon-separated list of locale to generate targets for.
# 'default_po_encoding': default encoding for new initialized PO files.
# 'target_basename': basename of the PO targets
# 'create_po_target': create one unique target for all locale PO files
# 'create_po_locale_targets': create one target per locale PO file

macro(KWWidgets_CREATE_PO_TARGETS
    domain_name
    pot_build_dir
    po_dir
    po_build_dir
    po_prefix
    locale_list
    default_po_encoding
    target_basename
    create_po_target
    create_po_locale_targets
    )

  kwwidgets_get_pot_filename(pot_build_file 
    "${domain_name}" "${pot_build_dir}")

  file(MAKE_DIRECTORY ${po_build_dir})

  # We need dummy files that will just say: this PO target is up to date as 
  # far as its dependencies are concerned. This will prevent the PO
  # targets to be triggered again and again because the POT file is older
  # than the PO, but the PO does not really need to be changed, etc.

  kwwidgets_get_po_safe_build_dir(safe_build_dir "${po_dir}" "${po_build_dir}")

  kwwidgets_get_pot_filename(pot_uptodate_file 
    "${domain_name}" "${safe_build_dir}")
  set(pot_uptodate_file "${pot_uptodate_file}.upd")

  set(po_build_files)

  foreach(locale ${locale_list})
    kwwidgets_get_po_filename(po_file 
      "${po_dir}" "${po_prefix}" "${locale}")
    kwwidgets_get_po_filename(po_build_file 
      "${po_build_dir}" "${po_prefix}" "${locale}")
    kwwidgets_get_po_filename(po_uptodate_file 
      "${safe_build_dir}" "${po_prefix}" "${locale}")
    set(po_uptodate_file "${po_uptodate_file}.upd")
    set(po_uptodate_files ${po_uptodate_files} ${po_uptodate_file})
    set(depends "${pot_uptodate_file}")
    if(EXISTS "${po_file}")
      set(depends ${depends} "${po_file}")
    endif(EXISTS "${po_file}")
    add_custom_command(
      OUTPUT "${po_uptodate_file}"
      DEPENDS ${depends}
      COMMAND ${CMAKE_COMMAND} 
      ARGS -DCMAKE_BACKWARDS_COMPATIBILITY:STRING=${CMAKE_BACKWARDS_COMPATIBILITY} -Dpo_file:STRING=${po_file} -Dpo_build_file:STRING=${po_build_file} -Dpo_uptodate_file:STRING=${po_uptodate_file} -Ddefault_po_encoding:STRING=${default_po_encoding} -Dpot_build_file:STRING=${pot_build_file} -Dlocale:STRING=${locale} -DGETTEXT_MSGINIT_EXECUTABLE:STRING=${GETTEXT_MSGINIT_EXECUTABLE} -DGETTEXT_MSGCONV_EXECUTABLE:STRING=${GETTEXT_MSGCONV_EXECUTABLE} -DGETTEXT_MSGMERGE_EXECUTABLE:STRING=${GETTEXT_MSGMERGE_EXECUTABLE} -DGETTEXT_MSGCAT_EXECUTABLE:STRING=${GETTEXT_MSGCAT_EXECUTABLE} -P "${KWWidgets_CMAKE_DIR}/KWWidgetsGettextInitOrMerge.cmake"
      )
    if(create_po_locale_targets)
      add_custom_target(
        ${target_basename}_po_${locale} DEPENDS ${po_uptodate_file})
    endif(create_po_locale_targets)
  endforeach(locale ${locale_list})

  if(create_po_target)
    add_custom_target(${target_basename}_po DEPENDS ${po_uptodate_files})
  endif(create_po_target)

endmacro(KWWidgets_CREATE_PO_TARGETS)

# ---------------------------------------------------------------------------
# KWWidgets_CREATE_MO_TARGETS
# Given a domain name, the location of a PO directory, a list of locales, the
# location of a MO build and install dir, create multiple custom 
# commands/targets to compile the translation files (po) for each locale into
# a binary translation files (mo). Each translation file is found in the
# PO directory as 'locale.po' (say, fr.po) and compiled into a binary 
# translation file in 'mo_build_dir'/locale/LC_MESSAGES/'domain_name'.mo 
# (say, 'mo_build_dir'/fr/LC_MESSAGES/'domain_name'.mo).
# 'domain_name': translation domain name (i.e. name of application or library)
# 'po_dir': path to where the original PO file are found
# 'po_build_dir': build path to where up-to-date PO files are stored
# 'po_prefix': string that is used to prefix each translation file.
# 'locale_list': semicolon-separated list of locale to generate targets for.
# 'mo_build_dir': directory where the binary MO files should be saved to
# 'mo_install_dir': directory where the binary MO files should be installed to
# 'target_basename': basename of the MO targets
# 'create_mo_target': create one unique target for all locale MO files
# 'create_mo_locale_targets': create one target per locale MO file
# 'add_mo_target_to_all': if true, add the unique MO target to the 'ALL' target

macro(KWWidgets_CREATE_MO_TARGETS
    domain_name
    po_dir
    po_build_dir
    po_prefix
    locale_list
    mo_build_dir
    mo_install_dir
    target_basename
    create_mo_target
    create_mo_locale_targets
    add_mo_target_to_all
    )

  set(mo_files)

  kwwidgets_get_po_safe_build_dir(safe_build_dir "${po_dir}" "${po_build_dir}")

  if(NOT "${GETTEXT_MSGFMT_EXECUTABLE}" STREQUAL "")

    foreach(locale ${locale_list})
      kwwidgets_get_po_filename(po_build_file 
        "${po_build_dir}" "${po_prefix}" "${locale}")
      kwwidgets_get_po_filename(po_uptodate_file 
        "${safe_build_dir}" "${po_prefix}" "${locale}")
      set(po_uptodate_file "${po_uptodate_file}.upd")
      kwwidgets_get_mo_filename(mo_file 
        "${domain_name}" "${mo_build_dir}" "${locale}")
      get_filename_component(mo_dir "${mo_file}" PATH)
      file(MAKE_DIRECTORY ${mo_dir})
      set(mo_files ${mo_files} ${mo_file})
      # --check-accelerators 
      add_custom_command(
        OUTPUT "${mo_file}"
        DEPENDS "${po_uptodate_file}"
        COMMAND ${GETTEXT_MSGFMT_EXECUTABLE} 
        ARGS --output-file=${mo_file} --check-format "${po_build_file}"
        )
      if(create_mo_locale_targets)
        add_custom_target(${target_basename}_mo_${locale} DEPENDS ${mo_file})
      endif(create_mo_locale_targets)
      
      if(NOT "${mo_install_dir}" STREQUAL "")
        install_files(
          "${mo_install_dir}/${locale}/LC_MESSAGES" FILES ${mo_file})
      endif(NOT "${mo_install_dir}" STREQUAL "")
    endforeach(locale ${locale_list})

    if(create_mo_target OR add_mo_target_to_all)
      set(target_name "${target_basename}_mo")
      if(add_mo_target_to_all)
        add_custom_target(${target_name} ALL DEPENDS ${mo_files})
      else(add_mo_target_to_all)
        add_custom_target(${target_name} DEPENDS ${mo_files})
      endif(add_mo_target_to_all)
    endif(create_mo_target OR add_mo_target_to_all)
    
  endif(NOT "${GETTEXT_MSGFMT_EXECUTABLE}" STREQUAL "")

endmacro(KWWidgets_CREATE_MO_TARGETS)
