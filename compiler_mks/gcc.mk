ifneq ($(USE_DEFAULT_CC), TRUE)
CC          = gcc
AR          = ar
CXX         = g++
endif
ARFLAGS     = -rcs
CFLAGS     += -W -Wall
DEBUG_FLAG += -g