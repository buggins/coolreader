# This file is a helper file for the gettext translation macros found in
# KWWidgetsInternationalizationMacros.cmake.
# If is used as a custom command in the KWWidgets_CREATE_POT_TARGETS macro.
# This macro extracts translatable strings out of source files into a POT
# file (template translation fiel). The problem is that even if no changes
# occurred as far as the translation strings are concerned, xgettext will
# always create a new file with a different POT-Creation-Date field. This
# forces all the depending targets to be updated when they do not really have
# to. Fix that by comparing the next POT file to the old one without taking
# the POT-Creation-Date into account.
#
# 'pot_build_file' (string): the POT file the strings should be extracted to
# 'pot_uptodate_file' (string): the dummy file which will be up to date
# 'options' (string): options
# 'keywords' (string): keywords
# 'copyright_holder': copyright holder of the template file
# 'msgid_bugs_address': report address for msgid bugs
# 'files_from': 
# GETTEXT_XGETTEXT_EXECUTABLE (string): path to the 'xgettext' executable

set(SUCCESS 1)
if(NOT "${GETTEXT_XGETTEXT_EXECUTABLE}" STREQUAL "")

  # Extract the strings, store the result in a variable instead of a POT file

  exec_program(${GETTEXT_XGETTEXT_EXECUTABLE} 
    RETURN_VALUE xgettext_return
    OUTPUT_VARIABLE xgettext_output
    ARGS --output="-" ${options} ${keywords} --msgid-bugs-address="${msgid_bugs_address}" --copyright-holder="${copyright_holder}" --files-from="${files_from}")
  if(xgettext_return)
    message("${xgettext_output}")
    set(SUCCESS 0)
  else(xgettext_return)

    set(xgettext_output "${xgettext_output}\n")

    # Check if the new POT file would be different than the old one
    # without taking into account the POT-Creation-Date.

    set(update_pot_file 0)
    if(EXISTS ${pot_build_file})
      string(REGEX REPLACE "\"POT-Creation-Date:[^\"]*\"" "" 
        xgettext_output_nodate "${xgettext_output}")
      file(READ "${pot_build_file}" xgettext_old)
      string(REGEX REPLACE "\"POT-Creation-Date:[^\"]*\"" "" 
        xgettext_old_nodate "${xgettext_old}")
      if(NOT "${xgettext_output_nodate}" STREQUAL "${xgettext_old_nodate}")
        set(update_pot_file 1)
      endif(NOT "${xgettext_output_nodate}" STREQUAL "${xgettext_old_nodate}")
    else(EXISTS ${pot_build_file})
      set(update_pot_file 1)
    endif(EXISTS ${pot_build_file})

    # Create the POT file if it is really needed

    if(update_pot_file)
      message("Updating ${pot_build_file}")
      file(WRITE "${pot_build_file}" "${xgettext_output}")
    endif(update_pot_file)

    # Update the dummy file to say: this POT target is up to date as
    # far as its dependencies are concerned. This will prevent the POT
    # target to be triggered again and again because the sources are older
    # than the POT, but the POT does not really need to be changed, etc.

    if(SUCCESS)
      file(WRITE "${pot_uptodate_file}" 
        "${pot_build_file} is *really* up-to-date.")
    endif(SUCCESS)

  endif(xgettext_return)
endif(NOT "${GETTEXT_XGETTEXT_EXECUTABLE}" STREQUAL "")
