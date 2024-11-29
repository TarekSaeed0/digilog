cmake_minimum_required(VERSION 3.30.4)
project(
	digilog
	VERSION 1.0
	LANGUAGES C
	DESCRIPTION "A program for digital logic design written in c"
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(digilog src/environment.c src/expression.c src/main.c)
target_include_directories(digilog PRIVATE include)
target_compile_options(
	digilog
	PRIVATE -Werror
			-Wall
			-Wextra
			-pedantic
			-Wfloat-equal
			-Wundef
			-Wshadow
			-Wpointer-arith
			-Wcast-align
			-Wswitch-default
			-Wstrict-prototypes
			-Wstrict-overflow=5
			-Wwrite-strings
			-Wcast-qual
			-Wconversion
			-fsanitize=address
			-fsanitize=undefined
)
target_link_options(digilog PRIVATE -fsanitize=address -fsanitize=undefined)
