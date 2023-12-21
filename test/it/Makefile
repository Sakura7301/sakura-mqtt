## PATH
ROOT_PATH 		= $(shell pwd)
DIST_PATH 		= $(ROOT_PATH)/dist
DOT_CONFIG_FILE = $(ROOT_PATH)/../../.config
LIBS_PATH 		= $(ROOT_PATH)/../../dist
RPATH    		= -Wl,-rpath=$(LIBS_PATH)

######################################
# source files
######################################
SOURCE_FILES-y  = $(ROOT_PATH)/testmain.c \
                $(ROOT_PATH)/../mock/mock_api.c \
				$(ROOT_PATH)/../common/common.c \
                $(ROOT_PATH)/testcases/test_mqtt_client.c \
				$(ROOT_PATH)/testcases/test_mqtt_net.c \
				$(ROOT_PATH)/testcases/test_register.c \
				$(ROOT_PATH)/testcases/test_data_point.c \
				$(ROOT_PATH)/testcases/test_public_interface.c \
				$(ROOT_PATH)/testcases/test_mqtt_multithread.c \
				$(ROOT_PATH)/testcases/test_dfile.c
				

######################################
# objects
######################################
OBJS  := $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SOURCE_FILES-y)))
TARGET_OBJS := $(foreach x,$(OBJS),$(addprefix $(DIST_PATH)/,$(notdir $(x))))


######################################
# include path
######################################
INCLUDE_PATH-y = -I $(ROOT_PATH)/../inc -I $(ROOT_PATH)/../mock/ -I $(ROOT_PATH)/../../src/ -I $(ROOT_PATH)/../../src/common -I $(ROOT_PATH)/../../src/protocol/dns -I $(ROOT_PATH)/../../src/protocol/http
INCLUDE_PATH-$(CONFIG_LOG) += -I $(ROOT_PATH)/../../src/log/

######################################
# compile options
######################################
CFLAGS += $(INCLUDE_PATH-y) -fpermissive -Wno-narrowing -std=c++11 -o0
LDFLAGS += $(RPATH) -lsakura_mqtt -lcunit -pthread


CXXFLAGS = $(CFLAGS)
FILTER_FLAGS=-Werror -Wno-unused-variable -Wno-unused-function -Wno-misleading-indentation -Wno-unused-but-set-variable -Wno-missing-braces
CXXFLAGS := $(filter-out $(FILTER_FLAGS),$(CXXFLAGS))

#i think you should do anything here
.PHONY: objs clean test it check_path

all: it

check_path:
	-mkdir $(DIST_PATH)

%.o: %.c
	$(CXX) -c $(CXXFLAGS) $< -o $(DIST_PATH)/$(notdir $@)

objs: $(OBJS)

clean:
	-rm -rf $(DIST_PATH)
	-rm -rf $(ROOT_PATH)/it

it: check_path $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(ROOT_PATH)/it $(TARGET_OBJS) -L $(LIBS_PATH) $(LDFLAGS) $(TEST_LDFLAGS)