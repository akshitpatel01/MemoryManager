# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.22

# Default target executed when no arguments are given to make.
default_target: all
.PHONY : default_target

# Allow only one "make -f Makefile2" at a time, but pass parallelism.
.NOTPARALLEL:

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
CMAKE_SOURCE_DIR = /home/akshit/MemoryManager

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/akshit/MemoryManager

#=============================================================================
# Targets provided globally by CMake.

# Special rule for the target edit_cache
edit_cache:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "No interactive CMake dialog available..."
	/usr/bin/cmake -E echo No\ interactive\ CMake\ dialog\ available.
.PHONY : edit_cache

# Special rule for the target edit_cache
edit_cache/fast: edit_cache
.PHONY : edit_cache/fast

# Special rule for the target rebuild_cache
rebuild_cache:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "Running CMake to regenerate build system..."
	/usr/bin/cmake --regenerate-during-build -S$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR)
.PHONY : rebuild_cache

# Special rule for the target rebuild_cache
rebuild_cache/fast: rebuild_cache
.PHONY : rebuild_cache/fast

# The main all target
all: cmake_check_build_system
	$(CMAKE_COMMAND) -E cmake_progress_start /home/akshit/MemoryManager/CMakeFiles /home/akshit/MemoryManager//CMakeFiles/progress.marks
	$(MAKE) $(MAKESILENT) -f CMakeFiles/Makefile2 all
	$(CMAKE_COMMAND) -E cmake_progress_start /home/akshit/MemoryManager/CMakeFiles 0
.PHONY : all

# The main clean target
clean:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/Makefile2 clean
.PHONY : clean

# The main clean target
clean/fast: clean
.PHONY : clean/fast

# Prepare targets for installation.
preinstall: all
	$(MAKE) $(MAKESILENT) -f CMakeFiles/Makefile2 preinstall
.PHONY : preinstall

# Prepare targets for installation.
preinstall/fast:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/Makefile2 preinstall
.PHONY : preinstall/fast

# clear depends
depend:
	$(CMAKE_COMMAND) -S$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR) --check-build-system CMakeFiles/Makefile.cmake 1
.PHONY : depend

#=============================================================================
# Target rules for targets named hash_lib

# Build rule for target.
hash_lib: cmake_check_build_system
	$(MAKE) $(MAKESILENT) -f CMakeFiles/Makefile2 hash_lib
.PHONY : hash_lib

# fast build rule for target.
hash_lib/fast:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/hash_lib.dir/build.make CMakeFiles/hash_lib.dir/build
.PHONY : hash_lib/fast

#=============================================================================
# Target rules for targets named test

# Build rule for target.
test: cmake_check_build_system
	$(MAKE) $(MAKESILENT) -f CMakeFiles/Makefile2 test
.PHONY : test

# fast build rule for target.
test/fast:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/test.dir/build.make CMakeFiles/test.dir/build
.PHONY : test/fast

Main.o: Main.cpp.o
.PHONY : Main.o

# target to build an object file
Main.cpp.o:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/test.dir/build.make CMakeFiles/test.dir/Main.cpp.o
.PHONY : Main.cpp.o

Main.i: Main.cpp.i
.PHONY : Main.i

# target to preprocess a source file
Main.cpp.i:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/test.dir/build.make CMakeFiles/test.dir/Main.cpp.i
.PHONY : Main.cpp.i

Main.s: Main.cpp.s
.PHONY : Main.s

# target to generate assembly for a file
Main.cpp.s:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/test.dir/build.make CMakeFiles/test.dir/Main.cpp.s
.PHONY : Main.cpp.s

src/hash_linear.o: src/hash_linear.cpp.o
.PHONY : src/hash_linear.o

# target to build an object file
src/hash_linear.cpp.o:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/hash_lib.dir/build.make CMakeFiles/hash_lib.dir/src/hash_linear.cpp.o
.PHONY : src/hash_linear.cpp.o

src/hash_linear.i: src/hash_linear.cpp.i
.PHONY : src/hash_linear.i

# target to preprocess a source file
src/hash_linear.cpp.i:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/hash_lib.dir/build.make CMakeFiles/hash_lib.dir/src/hash_linear.cpp.i
.PHONY : src/hash_linear.cpp.i

src/hash_linear.s: src/hash_linear.cpp.s
.PHONY : src/hash_linear.s

# target to generate assembly for a file
src/hash_linear.cpp.s:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/hash_lib.dir/build.make CMakeFiles/hash_lib.dir/src/hash_linear.cpp.s
.PHONY : src/hash_linear.cpp.s

src/util.o: src/util.c.o
.PHONY : src/util.o

# target to build an object file
src/util.c.o:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/hash_lib.dir/build.make CMakeFiles/hash_lib.dir/src/util.c.o
.PHONY : src/util.c.o

src/util.i: src/util.c.i
.PHONY : src/util.i

# target to preprocess a source file
src/util.c.i:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/hash_lib.dir/build.make CMakeFiles/hash_lib.dir/src/util.c.i
.PHONY : src/util.c.i

src/util.s: src/util.c.s
.PHONY : src/util.s

# target to generate assembly for a file
src/util.c.s:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/hash_lib.dir/build.make CMakeFiles/hash_lib.dir/src/util.c.s
.PHONY : src/util.c.s

# Help Target
help:
	@echo "The following are some of the valid targets for this Makefile:"
	@echo "... all (the default if no target is provided)"
	@echo "... clean"
	@echo "... depend"
	@echo "... edit_cache"
	@echo "... rebuild_cache"
	@echo "... hash_lib"
	@echo "... test"
	@echo "... Main.o"
	@echo "... Main.i"
	@echo "... Main.s"
	@echo "... src/hash_linear.o"
	@echo "... src/hash_linear.i"
	@echo "... src/hash_linear.s"
	@echo "... src/util.o"
	@echo "... src/util.i"
	@echo "... src/util.s"
.PHONY : help



#=============================================================================
# Special targets to cleanup operation of make.

# Special rule to run CMake to check the build system integrity.
# No rule that depends on this can have commands that come from listfiles
# because they might be regenerated.
cmake_check_build_system:
	$(CMAKE_COMMAND) -S$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR) --check-build-system CMakeFiles/Makefile.cmake 0
.PHONY : cmake_check_build_system
