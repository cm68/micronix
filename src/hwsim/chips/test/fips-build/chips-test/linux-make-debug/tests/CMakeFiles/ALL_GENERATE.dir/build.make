# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.13

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/curt/src/micronix/hwsim/chips/chips-test

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/curt/src/micronix/hwsim/chips/fips-build/chips-test/linux-make-debug

# Utility rule file for ALL_GENERATE.

# Include the progress variables for this target.
include tests/CMakeFiles/ALL_GENERATE.dir/progress.make

tests/CMakeFiles/ALL_GENERATE:
	cd /home/curt/src/micronix/hwsim/chips/chips-test && /usr/bin/python /home/curt/src/micronix/hwsim/chips/fips-build/chips-test/linux-make-debug/fips-gen.py /home/curt/src/micronix/hwsim/chips/fips-build/chips-test/linux-make-debug/fips_codegen.yml

ALL_GENERATE: tests/CMakeFiles/ALL_GENERATE
ALL_GENERATE: tests/CMakeFiles/ALL_GENERATE.dir/build.make

.PHONY : ALL_GENERATE

# Rule to build all files generated by this target.
tests/CMakeFiles/ALL_GENERATE.dir/build: ALL_GENERATE

.PHONY : tests/CMakeFiles/ALL_GENERATE.dir/build

tests/CMakeFiles/ALL_GENERATE.dir/clean:
	cd /home/curt/src/micronix/hwsim/chips/fips-build/chips-test/linux-make-debug/tests && $(CMAKE_COMMAND) -P CMakeFiles/ALL_GENERATE.dir/cmake_clean.cmake
.PHONY : tests/CMakeFiles/ALL_GENERATE.dir/clean

tests/CMakeFiles/ALL_GENERATE.dir/depend:
	cd /home/curt/src/micronix/hwsim/chips/fips-build/chips-test/linux-make-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/curt/src/micronix/hwsim/chips/chips-test /home/curt/src/micronix/hwsim/chips/chips-test/tests /home/curt/src/micronix/hwsim/chips/fips-build/chips-test/linux-make-debug /home/curt/src/micronix/hwsim/chips/fips-build/chips-test/linux-make-debug/tests /home/curt/src/micronix/hwsim/chips/fips-build/chips-test/linux-make-debug/tests/CMakeFiles/ALL_GENERATE.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : tests/CMakeFiles/ALL_GENERATE.dir/depend
