# This file is a helper file for the gettext translation macros found in
# KWWidgetsInternationalizationMacros.cmake.
# If is used as a custom command in the KWWidgets_CREATE_PO_TARGETS macro.
# This macro refreshes a PO file when its dependency, the POT template file,
# has changed. The problem is that if no PO file has ever been created yet,
# the 'msginit' executable should be used to initialize the PO file from
# the POT file. If it has been created already, the 'msgmerge' executable 
# should be used to merge the PO file with the current POT file.
# Since ADD_CUSTOM_COMMAND does not support such logic, it will use
# this file instead, and execute it using CMake with the relevant parameters.
# Given the parameters, this file will either use 'msginit' or 'msgmerge'.
#
# 'po_file' (string): the original PO file in the source tree, if any
# 'po_build_file' (string): the build PO file to initialize or merge
# 'po_uptodate_file' (string): the dummy file which will be up to date
# 'default_po_encoding' (string): default encoding to initialize PO file with
# 'pot_build_file' (string): the POT file this PO file depends on
# 'locale' (string): the locale this PO file represents (say, "fr")
# GETTEXT_MSGINIT_EXECUTABLE (string): path to the 'msginit' executable
# GETTEXT_MSGCONV_EXECUTABLE (string): path to the 'msgconv' executable 
# GETTEXT_MSGMERGE_EXECUTABLE (string): path to the 'msgmerge' executable 

set(SUCCESS 1)
if(NOT EXISTS "${po_build_file}")

  # Initialize PO file or copy from existing PO file

  if(EXISTS "${po_file}")
    message("Initializing PO file ${po_build_file} as a copy of ${po_file}")
    configure_file("${po_file}" "${po_build_file}" COPYONLY)
  else(EXISTS "${po_file}")
    if(NOT "${GETTEXT_MSGINIT_EXECUTABLE}" STREQUAL "")

      # Initialize PO file

      message("Initializing PO file ${po_build_file}")
      exec_program(${GETTEXT_MSGINIT_EXECUTABLE} 
        RETURN_VALUE msginit_return
        OUTPUT_VARIABLE msginit_output
        ARGS --no-translator --input="${pot_build_file}" --output-file="${po_build_file}" --locale="${locale}")
      if(msginit_output)
        if(NOT WIN32 OR NOT "${msginit_output}" MATCHES "Bad file desc")
          message("${msginit_output}")
          set(SUCCESS 0)
        endif(NOT WIN32 OR NOT "${msginit_output}" MATCHES "Bad file desc")
      endif(msginit_output)

      # Change initialized PO file encoding

      if(NOT "${GETTEXT_MSGCONV_EXECUTABLE}" STREQUAL "")
        if(NOT default_po_encoding)
          set(default_po_encoding "utf-8")
        endif(NOT default_po_encoding)
        exec_program(${GETTEXT_MSGCONV_EXECUTABLE} 
          RETURN_VALUE msgconv_return
          OUTPUT_VARIABLE msgconv_output
          ARGS --output-file="${po_build_file}" --to-code="${default_po_encoding}" \"${po_build_file}\")
        if(msgconv_output)
          message("${msgconv_output}")
          set(SUCCESS 0)
        endif(msgconv_output)
      endif(NOT "${GETTEXT_MSGCONV_EXECUTABLE}" STREQUAL "")

    endif(NOT "${GETTEXT_MSGINIT_EXECUTABLE}" STREQUAL "")

  endif(EXISTS "${po_file}")

else(NOT EXISTS "${po_build_file}")

  # Merge PO file with POT file

  #MESSAGE("Merging PO file ${po_build_file} with POT file ${pot_build_file}")
  # --output-file and --update are mutually exclusive. If --update is
  # specified, the PO file will not be re-written if the result of
  # the merge produces no modification. This can be problematic if the POT
  # file is newer than the PO file, and a MO file is generated from the PO
  # file: this seems to force the MO to always be regenerated.
  # --output-file=${po_build_file}
  # UPDATE: apparently --update still touches the file... 

  if(NOT "${GETTEXT_MSGMERGE_EXECUTABLE}" STREQUAL "")

    # Merge the strings, store the result in a variable instead of a PO file

    exec_program(${GETTEXT_MSGMERGE_EXECUTABLE} 
      RETURN_VALUE msgmerge_return
      OUTPUT_VARIABLE msgmerge_output
      ARGS --quiet --output-file="-" \"${po_build_file}\" \"${pot_build_file}\")
    if(msgmerge_return)
      message("${msgmerge_output}")
      set(SUCCESS 0)
    endif(msgmerge_return)

    set(msgmerge_output "${msgmerge_output}\n")

    # Check if the new PO file would be different than the old one

    set(update_po_file 0)
    if(EXISTS ${po_build_file})
      file(READ "${po_build_file}" po_old)
      if(NOT "${msgmerge_output}" STREQUAL "${po_old}")
        set(update_po_file 1)
      endif(NOT "${msgmerge_output}" STREQUAL "${po_old}")
    else(EXISTS ${po_build_file})
      set(update_po_file 1)
    endif(EXISTS ${po_build_file})

    # Create the PO file if it is really needed

    if(update_po_file)
      message("Updating ${po_build_file}")
      file(WRITE "${po_build_file}" "${msgmerge_output}")
    endif(update_po_file)

  endif(NOT "${GETTEXT_MSGMERGE_EXECUTABLE}" STREQUAL "")

  # msggrep.exe -T -e "#-#-#" would have done the trick but not in 0.13.1
  #   IF(NOT "${GETTEXT_MSGCAT_EXECUTABLE}" STREQUAL "" AND EXISTS "${po_file}")
  #     EXEC_PROGRAM(${GETTEXT_MSGCAT_EXECUTABLE} 
  #       ARGS "${po_build_file}" "${po_file}"
  #       OUTPUT_VARIABLE msgcat_output)
  #     STRING(REGEX MATCH "^\"#-#.*$" matched "${msgcat_output}")
  #     MESSAGE("match: ${matched}")
  # #    STRING(REGEX REPLACE "^(Python )([0-9]\\.[0-9])(.*)$" "\\2" 
  #  #     major_minor "${version}")

  #   ENDIF(NOT "${GETTEXT_MSGCAT_EXECUTABLE}" STREQUAL "" AND EXISTS "${po_file}")
endif(NOT EXISTS "${po_build_file}")

# Update the dummy file to say: this PO target is up to date as 
# far as its dependencies are concerned. This will prevent the PO
# targets to be triggered again and again because the POT file is older
# than the PO, but the PO does not really need to be changed, etc.

if(SUCCESS)
  file(WRITE "${po_uptodate_file}" 
    "${po_build_file} is *really* up-to-date.")
endif(SUCCESS)

