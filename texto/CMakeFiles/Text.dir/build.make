# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.22

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
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
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/gabriel/UA/4ano/IC/project1/texto

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/gabriel/UA/4ano/IC/project1/texto

# Include any dependencies generated for this target.
include CMakeFiles/Text.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/Text.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/Text.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/Text.dir/flags.make

CMakeFiles/Text.dir/main.cpp.o: CMakeFiles/Text.dir/flags.make
CMakeFiles/Text.dir/main.cpp.o: main.cpp
CMakeFiles/Text.dir/main.cpp.o: CMakeFiles/Text.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/gabriel/UA/4ano/IC/project1/texto/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/Text.dir/main.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/Text.dir/main.cpp.o -MF CMakeFiles/Text.dir/main.cpp.o.d -o CMakeFiles/Text.dir/main.cpp.o -c /home/gabriel/UA/4ano/IC/project1/texto/main.cpp

CMakeFiles/Text.dir/main.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/Text.dir/main.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/gabriel/UA/4ano/IC/project1/texto/main.cpp > CMakeFiles/Text.dir/main.cpp.i

CMakeFiles/Text.dir/main.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/Text.dir/main.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/gabriel/UA/4ano/IC/project1/texto/main.cpp -o CMakeFiles/Text.dir/main.cpp.s

# Object files for target Text
Text_OBJECTS = \
"CMakeFiles/Text.dir/main.cpp.o"

# External object files for target Text
Text_EXTERNAL_OBJECTS =

Text: CMakeFiles/Text.dir/main.cpp.o
Text: CMakeFiles/Text.dir/build.make
Text: /usr/lib/x86_64-linux-gnu/libboost_locale.so.1.74.0
Text: /usr/lib/x86_64-linux-gnu/libpython3.10.so
Text: /usr/lib/x86_64-linux-gnu/libboost_chrono.so.1.74.0
Text: /usr/lib/x86_64-linux-gnu/libboost_system.so.1.74.0
Text: /usr/lib/x86_64-linux-gnu/libboost_thread.so.1.74.0
Text: /usr/lib/x86_64-linux-gnu/libboost_atomic.so.1.74.0
Text: CMakeFiles/Text.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/gabriel/UA/4ano/IC/project1/texto/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable Text"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/Text.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/Text.dir/build: Text
.PHONY : CMakeFiles/Text.dir/build

CMakeFiles/Text.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/Text.dir/cmake_clean.cmake
.PHONY : CMakeFiles/Text.dir/clean

CMakeFiles/Text.dir/depend:
	cd /home/gabriel/UA/4ano/IC/project1/texto && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/gabriel/UA/4ano/IC/project1/texto /home/gabriel/UA/4ano/IC/project1/texto /home/gabriel/UA/4ano/IC/project1/texto /home/gabriel/UA/4ano/IC/project1/texto /home/gabriel/UA/4ano/IC/project1/texto/CMakeFiles/Text.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/Text.dir/depend

