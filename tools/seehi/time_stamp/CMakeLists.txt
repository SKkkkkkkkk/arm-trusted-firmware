cmake_minimum_required(VERSION 3.15...3.26)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(EXE_NAME time_stamp_interpreter)

project("${EXE_NAME}"
  VERSION 0.1.0
  DESCRIPTION "${EXE_NAME}"
  LANGUAGES C
)

add_executable(${EXE_NAME})

target_compile_options(${EXE_NAME}
	PRIVATE
		-Wall -Wextra -Werror
)

target_sources(${EXE_NAME}
	PRIVATE
		src/${EXE_NAME}.c
)

target_include_directories(${EXE_NAME}
	PUBLIC
		inc
)
