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

# Include any dependencies generated for this target.
include examples/common/CMakeFiles/common.dir/depend.make

# Include the progress variables for this target.
include examples/common/CMakeFiles/common.dir/progress.make

# Include the compile flags for this target's objects.
include examples/common/CMakeFiles/common.dir/flags.make

examples/common/CMakeFiles/common.dir/common.c.o: examples/common/CMakeFiles/common.dir/flags.make
examples/common/CMakeFiles/common.dir/common.c.o: /home/curt/src/micronix/hwsim/chips/chips-test/examples/common/common.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/curt/src/micronix/hwsim/chips/fips-build/chips-test/linux-make-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object examples/common/CMakeFiles/common.dir/common.c.o"
	cd /home/curt/src/micronix/hwsim/chips/fips-build/chips-test/linux-make-debug/examples/common && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/common.dir/common.c.o   -c /home/curt/src/micronix/hwsim/chips/chips-test/examples/common/common.c

examples/common/CMakeFiles/common.dir/common.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/common.dir/common.c.i"
	cd /home/curt/src/micronix/hwsim/chips/fips-build/chips-test/linux-make-debug/examples/common && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/curt/src/micronix/hwsim/chips/chips-test/examples/common/common.c > CMakeFiles/common.dir/common.c.i

examples/common/CMakeFiles/common.dir/common.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/common.dir/common.c.s"
	cd /home/curt/src/micronix/hwsim/chips/fips-build/chips-test/linux-make-debug/examples/common && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/curt/src/micronix/hwsim/chips/chips-test/examples/common/common.c -o CMakeFiles/common.dir/common.c.s

examples/common/CMakeFiles/common.dir/sokol.c.o: examples/common/CMakeFiles/common.dir/flags.make
examples/common/CMakeFiles/common.dir/sokol.c.o: /home/curt/src/micronix/hwsim/chips/chips-test/examples/common/sokol.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/curt/src/micronix/hwsim/chips/fips-build/chips-test/linux-make-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object examples/common/CMakeFiles/common.dir/sokol.c.o"
	cd /home/curt/src/micronix/hwsim/chips/fips-build/chips-test/linux-make-debug/examples/common && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/common.dir/sokol.c.o   -c /home/curt/src/micronix/hwsim/chips/chips-test/examples/common/sokol.c

examples/common/CMakeFiles/common.dir/sokol.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/common.dir/sokol.c.i"
	cd /home/curt/src/micronix/hwsim/chips/fips-build/chips-test/linux-make-debug/examples/common && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/curt/src/micronix/hwsim/chips/chips-test/examples/common/sokol.c > CMakeFiles/common.dir/sokol.c.i

examples/common/CMakeFiles/common.dir/sokol.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/common.dir/sokol.c.s"
	cd /home/curt/src/micronix/hwsim/chips/fips-build/chips-test/linux-make-debug/examples/common && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/curt/src/micronix/hwsim/chips/chips-test/examples/common/sokol.c -o CMakeFiles/common.dir/sokol.c.s

# Object files for target common
common_OBJECTS = \
"CMakeFiles/common.dir/common.c.o" \
"CMakeFiles/common.dir/sokol.c.o"

# External object files for target common
common_EXTERNAL_OBJECTS =

examples/common/libcommon.a: examples/common/CMakeFiles/common.dir/common.c.o
examples/common/libcommon.a: examples/common/CMakeFiles/common.dir/sokol.c.o
examples/common/libcommon.a: examples/common/CMakeFiles/common.dir/build.make
examples/common/libcommon.a: examples/common/CMakeFiles/common.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/curt/src/micronix/hwsim/chips/fips-build/chips-test/linux-make-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking C static library libcommon.a"
	cd /home/curt/src/micronix/hwsim/chips/fips-build/chips-test/linux-make-debug/examples/common && $(CMAKE_COMMAND) -P CMakeFiles/common.dir/cmake_clean_target.cmake
	cd /home/curt/src/micronix/hwsim/chips/fips-build/chips-test/linux-make-debug/examples/common && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/common.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
examples/common/CMakeFiles/common.dir/build: examples/common/libcommon.a

.PHONY : examples/common/CMakeFiles/common.dir/build

examples/common/CMakeFiles/common.dir/clean:
	cd /home/curt/src/micronix/hwsim/chips/fips-build/chips-test/linux-make-debug/examples/common && $(CMAKE_COMMAND) -P CMakeFiles/common.dir/cmake_clean.cmake
.PHONY : examples/common/CMakeFiles/common.dir/clean

examples/common/CMakeFiles/common.dir/depend:
	cd /home/curt/src/micronix/hwsim/chips/fips-build/chips-test/linux-make-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/curt/src/micronix/hwsim/chips/chips-test /home/curt/src/micronix/hwsim/chips/chips-test/examples/common /home/curt/src/micronix/hwsim/chips/fips-build/chips-test/linux-make-debug /home/curt/src/micronix/hwsim/chips/fips-build/chips-test/linux-make-debug/examples/common /home/curt/src/micronix/hwsim/chips/fips-build/chips-test/linux-make-debug/examples/common/CMakeFiles/common.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : examples/common/CMakeFiles/common.dir/depend

