# some functions supporting some abbreviations

# usage:
#
# dune_add_subdirs( <dir1> [<dir2>...]
#                   [EXCLUDE <exclude string> | NOEXCLUDE]
#
# adds for each <dir1>, <dir2>... a subdirectory except the subdirectory
# name is equal to <exclude string>.
#
# If no <exclude string> is given a default value "test" will be used.
#
# Neglect the default value by using the NOEXCLUDE option.
#
function(dune_add_subdirs)

  include(CMakeParseArguments)
  cmake_parse_arguments(SHORT "NOEXCLUDE" "EXCLUDE" "" ${ARGN})

  if( SHORT_NOEXCLUDE )
    foreach(i ${SHORT_UNPARSED_ARGUMENTS})
      add_subdirectory(${i})
    endforeach(i ${SHORT_UNPARSED_ARGUMENTS})
  else()
    if(NOT SHORT_EXCLUDE)
      #default
      set(SHORT_EXCLUDE "test")
    endif()
    foreach(i ${SHORT_UNPARSED_ARGUMENTS})
      if(${i} STREQUAL "${SHORT_EXCLUDE}")
        set(opt EXCLUDE_FROM_ALL)
      endif(${i} STREQUAL "${SHORT_EXCLUDE}")
      add_subdirectory(${i} ${opt})
    endforeach(i ${SHORT_UNPARSED_ARGUMENTS})
  endif()
endfunction(dune_add_subdirs)

# usage
#
# simple version:
#
# dune_add_test( <test1> [<test2>...]
#                [FAILTEST <ftest1> ...]
#                [COMPILEFAILTEST <cftest1> ... ] )
#
# Simply adds all test to the cmake testing framework.
#
# key words:
# FAILTEST         the following test should fail to be successfull
# COMPILFAILTEST   the following test does not compile
#
#
# advanced version (needed in dune-fem):
#
# dune_add_test( <test1> [<test2>...]
#                [NO_DEPENDENCY] <ntest1> [<ntest2>...]
#                [FAILTEST [NO_DEPENDENCY] <ftest1> ...]
#                [COMPILEFAILTEST <cftest1> ... ]
#                [DEPENDENCY_ONLY <dep1> [dep_2] ] )
#
# arguments as above and:
#
# DEPENDENCY_ONLY  do not add targets to the test, but add them as dependencies
# NO_DEPENDENCY    add targets to test, but not as dependency. Should be combined
#                  with targets or FAILTEST targets...
#
# All of these tests are not build during make all.
# Build these targets with make test.
function(dune_add_test)
  include(CMakeParseArguments)

  #parse first level
  cmake_parse_arguments(ADD_TEST "" ""
    "FAILTEST;COMPILEFAILTEST;DEPENDENCY_ONLY" ${ARGN})


  #parse second level
  cmake_parse_arguments(ADD_TEST_UNPARSED_ARGUMENTS "" "" "NO_DEPENDENCY" ${ADD_TEST_UNPARSED_ARGUMENTS})
  cmake_parse_arguments(ADD_TEST_FAILTEST "" "" "NO_DEPENDENCY" ${ADD_TEST_FAILTEST})
  cmake_parse_arguments(ADD_TEST_COMPILEFAILTEST "" "" "NO_DEPENDENCY" ${ADD_TEST_COMPILEFAILTEST})

  #delete key words form first level
  set(ADD_TEST_UNPARSED_ARGUMENTS "${ADD_TEST_UNPARSED_ARGUMENTS_UNPARSED_ARGUMENTS};${ADD_TEST_UNPARSED_ARGUMENTS_NO_DEPENDENCY}")
  set(ADD_TEST_FAILTEST "${ADD_TEST_FAILTEST_UNPARSED_ARGUMENTS};${ADD_TEST_FAILTEST_NO_DEPENDENCY}")
  set(ADD_TEST_COMPILEFAILTEST "${ADD_TEST_COMPILEFAILTEST_UNPARSED_ARGUMENTS};${ADD_TEST_COMPILEFAILTEST_NO_DEPENDENCY}")

  #add tests
  foreach(i ${ADD_TEST_UNPARSED_ARGUMENTS} ${ADD_TEST_FAILTEST})
    add_test(${i} ${i})
  endforeach(i ${ADD_TEST_UNPARSED_ARGUMENTS} ${ADD_TEST_FAILTEST})
  # compile tests that should fail
  foreach(i ${ADD_TEST_COMPILEFAILTEST})
    add_test(NAME ${i}
    COMMAND ${CMAKE_COMMAND} --build . --target ${_TEST} --config $<CONFIGURATION>)
  endforeach(i ${ADD_TEST_COMPILEFAILTEST})

  set(DEP_TESTS ${ADD_TEST_UNPARSED_ARGUMENTS_UNPARSED_ARGUMENTS}
                ${ADD_TEST_FAILTEST_UNPARSED_ARGUMENTS} )
  if(DEP_TESTS)
    #check first, whether target already exists
    get_directory_test_target(_test_target "${CMAKE_CURRENT_BINARY_DIR}")
    if( NOT TARGET ${_test_target} )
      add_directory_test_target(_test_target)
    endif()
    add_dependencies(${_test_target} ${DEP_TESTS})
  endif()

  #Set properties for failing tests
  set_tests_properties(${ADD_TEST_FAILTEST} ${ADD_TEST_COMPILEFAILTEST}
    PROPERTIES WILL_FAIL true)

endfunction(dune_add_test)

# compatibility with dune-fem master
function(dune_fem_add_test)
  dune_add_test( ${ARGN} )
endfunction(dune_fem_add_test)

#
# install given files into current include directory
#
function(dune_install)
  string(REPLACE "${CMAKE_SOURCE_DIR}" "${CMAKE_INSTALL_INCLUDEDIR}" _dest "${CMAKE_CURRENT_SOURCE_DIR}" )
  install(FILES ${ARGV} DESTINATION ${_dest})
endfunction(dune_install)
