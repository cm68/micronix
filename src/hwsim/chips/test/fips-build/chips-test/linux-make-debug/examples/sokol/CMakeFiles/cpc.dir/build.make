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
include examples/sokol/CMakeFiles/cpc.dir/depend.make

# Include the progress variables for this target.
include examples/sokol/CMakeFiles/cpc.dir/progress.make

# Include the compile flags for this target's objects.
include examples/sokol/CMakeFiles/cpc.dir/flags.make

examples/sokol/CMakeFiles/cpc.dir/cpc.c.o: examples/sokol/CMakeFiles/cpc.dir/flags.make
examples/sokol/CMakeFiles/cpc.dir/cpc.c.o: /home/curt/src/micronix/hwsim/chips/chips-test/examples/sokol/cpc.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/curt/src/micronix/hwsim/chips/fips-build/chips-test/linux-make-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object examples/sokol/CMakeFiles/cpc.dir/cpc.c.o"
	cd /home/curt/src/micronix/hwsim/chips/fips-build/chips-test/linux-make-debug/examples/sokol && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/cpc.dir/cpc.c.o   -c /home/curt/src/micronix/hwsim/chips/chips-test/examples/sokol/cpc.c

examples/sokol/CMakeFiles/cpc.dir/cpc.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/cpc.dir/cpc.c.i"
	cd /home/curt/src/micronix/hwsim/chips/fips-build/chips-test/linux-make-debug/examples/sokol && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/curt/src/micronix/hwsim/chips/chips-test/examples/sokol/cpc.c > CMakeFiles/cpc.dir/cpc.c.i

examples/sokol/CMakeFiles/cpc.dir/cpc.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/cpc.dir/cpc.c.s"
	cd /home/curt/src/micronix/hwsim/chips/fips-build/chips-test/linux-make-debug/examples/sokol && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/curt/src/micronix/hwsim/chips/chips-test/examples/sokol/cpc.c -o CMakeFiles/cpc.dir/cpc.c.s

# Object files for target cpc
cpc_OBJECTS = \
"CMakeFiles/cpc.dir/cpc.c.o"

# External object files for target cpc
cpc_EXTERNAL_OBJECTS =

/home/curt/src/micronix/hwsim/chips/fips-deploy/chips-test/linux-make-debug/cpc: examples/sokol/CMakeFiles/cpc.dir/cpc.c.o
/home/curt/src/micronix/hwsim/chips/fips-deploy/chips-test/linux-make-debug/cpc: examples/sokol/CMakeFiles/cpc.dir/build.make
/home/curt/src/micronix/hwsim/chips/fips-deploy/chips-test/linux-make-debug/cpc: examples/roms/libroms.a
/home/curt/src/micronix/hwsim/chips/fips-deploy/chips-test/linux-make-debug/cpc: examples/common/libcommon.a
/home/curt/src/micronix/hwsim/chips/fips-deploy/chips-test/linux-make-debug/cpc: examples/sokol/CMakeFiles/cpc.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/curt/src/micronix/hwsim/chips/fips-build/chips-test/linux-make-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C executable /home/curt/src/micronix/hwsim/chips/fips-deploy/chips-test/linux-make-debug/cpc"
	cd /home/curt/src/micronix/hwsim/chips/fips-build/chips-test/linux-make-debug/examples/sokol && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/cpc.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
examples/sokol/CMakeFiles/cpc.dir/build: /home/curt/src/micronix/hwsim/chips/fips-deploy/chips-test/linux-make-debug/cpc

.PHONY : examples/sokol/CMakeFiles/cpc.dir/build

examples/sokol/CMakeFiles/cpc.dir/clean:
	cd /home/curt/src/micronix/hwsim/chips/fips-build/chips-test/linux-make-debug/examples/sokol && $(CMAKE_COMMAND) -P CMakeFiles/cpc.dir/cmake_clean.cmake
.PHONY : examples/sokol/CMakeFiles/cpc.dir/clean

examples/sokol/CMakeFiles/cpc.dir/depend:
	cd /home/curt/src/micronix/hwsim/chips/fips-build/chips-test/linux-make-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/curt/src/micronix/hwsim/chips/chips-test /home/curt/src/micronix/hwsim/chips/chips-test/examples/sokol /home/curt/src/micronix/hwsim/chips/fips-build/chips-test/linux-make-debug /home/curt/src/micronix/hwsim/chips/fips-build/chips-test/linux-make-debug/examples/sokol /home/curt/src/micronix/hwsim/chips/fips-build/chips-test/linux-make-debug/examples/sokol/CMakeFiles/cpc.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : examples/sokol/CMakeFiles/cpc.dir/depend
