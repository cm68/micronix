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
include examples/ascii/CMakeFiles/c64-ascii.dir/depend.make

# Include the progress variables for this target.
include examples/ascii/CMakeFiles/c64-ascii.dir/progress.make

# Include the compile flags for this target's objects.
include examples/ascii/CMakeFiles/c64-ascii.dir/flags.make

examples/ascii/CMakeFiles/c64-ascii.dir/c64-ascii.c.o: examples/ascii/CMakeFiles/c64-ascii.dir/flags.make
examples/ascii/CMakeFiles/c64-ascii.dir/c64-ascii.c.o: /home/curt/src/micronix/hwsim/chips/chips-test/examples/ascii/c64-ascii.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/curt/src/micronix/hwsim/chips/fips-build/chips-test/linux-make-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object examples/ascii/CMakeFiles/c64-ascii.dir/c64-ascii.c.o"
	cd /home/curt/src/micronix/hwsim/chips/fips-build/chips-test/linux-make-debug/examples/ascii && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/c64-ascii.dir/c64-ascii.c.o   -c /home/curt/src/micronix/hwsim/chips/chips-test/examples/ascii/c64-ascii.c

examples/ascii/CMakeFiles/c64-ascii.dir/c64-ascii.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/c64-ascii.dir/c64-ascii.c.i"
	cd /home/curt/src/micronix/hwsim/chips/fips-build/chips-test/linux-make-debug/examples/ascii && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/curt/src/micronix/hwsim/chips/chips-test/examples/ascii/c64-ascii.c > CMakeFiles/c64-ascii.dir/c64-ascii.c.i

examples/ascii/CMakeFiles/c64-ascii.dir/c64-ascii.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/c64-ascii.dir/c64-ascii.c.s"
	cd /home/curt/src/micronix/hwsim/chips/fips-build/chips-test/linux-make-debug/examples/ascii && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/curt/src/micronix/hwsim/chips/chips-test/examples/ascii/c64-ascii.c -o CMakeFiles/c64-ascii.dir/c64-ascii.c.s

# Object files for target c64-ascii
c64__ascii_OBJECTS = \
"CMakeFiles/c64-ascii.dir/c64-ascii.c.o"

# External object files for target c64-ascii
c64__ascii_EXTERNAL_OBJECTS =

/home/curt/src/micronix/hwsim/chips/fips-deploy/chips-test/linux-make-debug/c64-ascii: examples/ascii/CMakeFiles/c64-ascii.dir/c64-ascii.c.o
/home/curt/src/micronix/hwsim/chips/fips-deploy/chips-test/linux-make-debug/c64-ascii: examples/ascii/CMakeFiles/c64-ascii.dir/build.make
/home/curt/src/micronix/hwsim/chips/fips-deploy/chips-test/linux-make-debug/c64-ascii: examples/roms/libroms.a
/home/curt/src/micronix/hwsim/chips/fips-deploy/chips-test/linux-make-debug/c64-ascii: examples/ascii/CMakeFiles/c64-ascii.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/curt/src/micronix/hwsim/chips/fips-build/chips-test/linux-make-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C executable /home/curt/src/micronix/hwsim/chips/fips-deploy/chips-test/linux-make-debug/c64-ascii"
	cd /home/curt/src/micronix/hwsim/chips/fips-build/chips-test/linux-make-debug/examples/ascii && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/c64-ascii.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
examples/ascii/CMakeFiles/c64-ascii.dir/build: /home/curt/src/micronix/hwsim/chips/fips-deploy/chips-test/linux-make-debug/c64-ascii

.PHONY : examples/ascii/CMakeFiles/c64-ascii.dir/build

examples/ascii/CMakeFiles/c64-ascii.dir/clean:
	cd /home/curt/src/micronix/hwsim/chips/fips-build/chips-test/linux-make-debug/examples/ascii && $(CMAKE_COMMAND) -P CMakeFiles/c64-ascii.dir/cmake_clean.cmake
.PHONY : examples/ascii/CMakeFiles/c64-ascii.dir/clean

examples/ascii/CMakeFiles/c64-ascii.dir/depend:
	cd /home/curt/src/micronix/hwsim/chips/fips-build/chips-test/linux-make-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/curt/src/micronix/hwsim/chips/chips-test /home/curt/src/micronix/hwsim/chips/chips-test/examples/ascii /home/curt/src/micronix/hwsim/chips/fips-build/chips-test/linux-make-debug /home/curt/src/micronix/hwsim/chips/fips-build/chips-test/linux-make-debug/examples/ascii /home/curt/src/micronix/hwsim/chips/fips-build/chips-test/linux-make-debug/examples/ascii/CMakeFiles/c64-ascii.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : examples/ascii/CMakeFiles/c64-ascii.dir/depend

