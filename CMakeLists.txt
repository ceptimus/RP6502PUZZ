cmake_minimum_required(VERSION 3.13)

project(MY-RP6502-PROJECT C ASM)

add_subdirectory(tools)

add_executable(puzz)
rp6502_executable(puzz)
target_sources(puzz PRIVATE
    src/main.c
    src/gfx.c
    src/mouse.c
    src/puzzle.c
    src/menu.c
)
