MAKENAME = "Concorrente"

MAIN = programa
MODULES = cQueue

SRC_DIR = src
INCL_DIR = include
BUILD_DIR = build
BIN_DIR = bin

CC = cc
PPFLAGS = -D_POSIX_C_SOURCE=200112L -D_XOPEN_SOURCE=600
CCFLAGS = -std=c11 -g -O0 -fsanitize=address -fsanitize=undefined -pthread -Wall -Wextra -Wpedantic -Werror
LDFLAGS = -fsanitize=address -fsanitize=undefined -pthread