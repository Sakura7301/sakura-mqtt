
## PATH
ROOT_PATH = $(shell pwd)
SYS = $(shell uname -s)
ARCH = $(shell uname -m)
DIST_PATH = $(ROOT_PATH)/dist
MKCONFIG = $(ROOT_PATH)/tools/mock_config
COMPILER_MKS_PATH = $(ROOT_PATH)/compiler_mks
AUTOCONFIG_H = $(ROOT_PATH)/inc/sakura_autoconfig.h
DOT_CONFIG_FILE = $(ROOT_PATH)/.config
PACKET_NAME="sakura-mqtt-sdk"
COMMIT_ID := $(shell git rev-parse HEAD)


ifeq ($(INCLUDE_CMD), )
INCLUDE_CMD=-I
endif

# include compile macros `.config`
-include $(DOT_CONFIG_FILE)
# by default GCC
-include $(COMPILER_MKS_PATH)/gcc.mk

######################################
# include path
######################################
INCLUDE_PATH-y = $(INCLUDE_CMD)"$(ROOT_PATH)/inc" \
 				$(INCLUDE_CMD)"$(ROOT_PATH)/src/protocol/http" \
				$(INCLUDE_CMD)"$(ROOT_PATH)/src/utils/cjson" \
				$(INCLUDE_CMD)"$(ROOT_PATH)/src/log" \
				$(INCLUDE_CMD)"$(ROOT_PATH)/src/utils"

ifeq ($(CONFIG_SYS_UNIX), y)
ifeq ($(CONFIG_IMPL_STATIC_MEMORY), y)
INCLUDE_PATH-$(CONFIG_IMPL_STATIC_MEMORY) += $(INCLUDE_CMD)"$(ROOT_PATH)/src/platform/unix/memory/static/heap_allocator"
endif# CONFIG_IMPL_STATIC_MEMORY
endif# CONFIG_SYS_UNIX

ifeq ($(CONFIG_SYS_UNIX), y)
INCLUDE_PATH-y += -I $(ROOT_PATH)/src/protocol/dns
endif


######################################
# source files
######################################
SOURCE_FILES-y  = $(ROOT_PATH)/src/sakura_mqtt_net.c \
                $(ROOT_PATH)/src/sakura_mqtt_core.c \
                $(ROOT_PATH)/src/sakura_mqtt_client.c \
                $(ROOT_PATH)/src/sakura_mqtt_pack.c \
				$(ROOT_PATH)/src/utils/cjson/cJSON.c \
				$(ROOT_PATH)/src/utils/sakura_utils.c


# unix platform
ifeq ($(CONFIG_SYS_UNIX), y)
SOURCE_FILES-$(CONFIG_DAEMON)                   += $(ROOT_PATH)/src/platform/unix/daemon/sakura_daemon.c
SOURCE_FILES-$(CONFIG_IMPL_MUTEX_LOCK)         	+= $(ROOT_PATH)/src/platform/unix/mutex/sakura_mutex.c
SOURCE_FILES-$(CONFIG_IMPL_ASYNC_SOCKET)        += $(ROOT_PATH)/src/platform/unix/socket/sakura_socket.c
SOURCE_FILES-y									+= $(ROOT_PATH)/src/protocol/dns/sakura_dns.c
# mem
ifeq ($(CONFIG_IMPL_STATIC_MEMORY), y)
SOURCE_FILES-y      							+= $(ROOT_PATH)/src/platform/unix/memory/static/sakura_memory.c \
												$(ROOT_PATH)/src/platform/unix/memory/static/heap_allocator/heap.c \
												$(ROOT_PATH)/src/platform/unix/memory/static/heap_allocator/llist.c
else
SOURCE_FILES-y                                  += $(ROOT_PATH)/src/platform/unix/memory/dynamic/sakura_memory.c
endif # CONFIG_IMPL_STATIC_MEMORY
else
SOURCE_FILES-$(CONFIG_IMPL_MUTEX_LOCK)         += $(ROOT_PATH)/src/platform/embed/mutex/sakura_mutex.c
SOURCE_FILES-y                                  += $(ROOT_PATH)/src/platform/embed/socket/sakura_socket_lifecycle.c
SOURCE_FILES-$(CONFIG_IMPL_ASYNC_SOCKET)        += $(ROOT_PATH)/src/platform/embed/socket/sakura_socket_impl.c
endif # CONFIG_SYS_UNIX

# add log module source files
#####################################
ifeq ($(CONFIG_LOG), y)
SOURCE_FILES-y += $(ROOT_PATH)/src/log/sakura_log.c
endif # CONFIG_LOG



EXTRA_CFLAGS = $(INCLUDE_PATH-y)
######################################
# objects
######################################
OBJS  := $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SOURCE_FILES-y)))
TARGET_OBJS := $(foreach x,$(OBJS),$(addprefix $(DIST_PATH)/,$(notdir $(x))))


######################################
# compilers
######################################
CC      := $(CROSS_COMPILE)$(CC)
AR      := $(CROSS_COMPILE)$(AR)
CXX     := $(CROSS_COMPILE)$(CXX)

TEST_LDFLAGS += -lsakura_mqtt -lcunit
RELEASE_PREFIX    ?= libsakura_mqtt
TARGET_SHARED_LIB ?= $(DIST_PATH)/$(RELEASE_PREFIX).so
TARGET_STATIC_LIB ?= $(DIST_PATH)/$(RELEASE_PREFIX).a

# TARGET
ifeq ($(SYS), Linux)
CFLAGS  += -fPIC
CFLAGS  += -Werror
TARGET   = $(TARGET_SHARED_LIB) $(TARGET_STATIC_LIB)
RPATH    = -Wl,-rpath=$(DIST_PATH)
else
TARGET   = $(TARGET_STATIC_LIB)
endif

ifeq ($(RELEASE), RELEASE)
CFLAGS += -DSAKURA_RELEASE
else
CFLAGS += $(DEBUG_FLAG)
endif

# add pthread lib
ifeq ($(SYS), Linux)
LDFLAGS += -pthread
endif


# support gcovr or notdir
ifneq ($(GCOVR), )
ifeq ($(SYS), Linux)
LDFLAGS += -lgcov
endif
CFLAGS  += -fprofile-arcs -ftest-coverage
# remove exceptions flag for test case exceptions
CFLAGS += -DSAKURA_TEST_NO_EXCEPTIONS --coverage -fno-exceptions
endif

# support gcovr or notdir
ifneq ($(GCOVR), )
ifeq ($(SYS), Linux)
LDFLAGS += -lgcov
TEST_LDFLAGS += -lgcov
endif
CFLAGS  += -fprofile-arcs -ftest-coverage
endif

CFLAGS  += $(EXTRA_CFLAGS)
LDFLAGS += $(EXTRA_LDFLAGS)

# To prevent the nuptial
LDFLAGS += -Wl,-Bsymbolic

CFLAGS += -D'MQTT_SDK_COMMIT_ID="$(COMMIT_ID)"'

####### export for compiling app #######
export CC AR CFLAGS LDFLAGS ARFLAGS
########################################

#i think you should do anything here
.PHONY: objs clean check_path $(TARGET_SHARED_LIB) lib

all: gen_autoconfig_header_file lib

mock_config:
	make -C $(ROOT_PATH)/tools

gen_autoconfig_header_file: mock_config
	$(MKCONFIG) $(ROOT_PATH) > $(AUTOCONFIG_H)

lib: check_path $(TARGET)

check_path:
	-mkdir $(DIST_PATH)

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $(DIST_PATH)/$(notdir $@)

objs: $(OBJS)

clean:
	rm -rf $(DIST_PATH)
	rm -rf demo
	rm -rf release
	rm -rf archive/include/*
	make -C $(ROOT_PATH)/tools clean
	make -C $(ROOT_PATH)/test/ut clean
	make -C $(ROOT_PATH)/test/it clean

$(TARGET_SHARED_LIB): $(OBJS)
	$(CC) $(CFLAGS) -shared -o $@ $(TARGET_OBJS) $(LDFLAGS)

$(TARGET_STATIC_LIB): $(OBJS)
	$(AR) $(ARFLAGS) $@ $(TARGET_OBJS)



demo: demo.c
	$(CC) $< -o $@ $(CFLAGS) -L $(DIST_PATH) $(LDFLAGS) -lsakura_mqtt $(RPATH) $(LDFLAGS)

ut:
	make -C $(ROOT_PATH)/test/ut

it:
	make -C $(ROOT_PATH)/test/it
