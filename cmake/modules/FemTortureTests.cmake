if("$ENV{TORTURE_TESTS}" STREQUAL "on")
  set(TORTURE_TESTS on CACHE BOOL "Enable Torture Tests")
  message(STATUS "Enabling torture-tests")
else()
  set(TORTURE_TESTS off CACHE BOOL "Enable Torture Tests")
  message(STATUS "Not enabling torture-tests")
endif()
