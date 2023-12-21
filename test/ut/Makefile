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
				$(ROOT_PATH)/testcases/test_client.c

######################################
# objects
######################################
OBJS  := $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SOURCE_FILES-y)))
TARGET_OBJS := $(foreach x,$(OBJS),$(addprefix $(DIST_PATH)/,$(notdir $(x))))


######################################
# include path
######################################
INCLUDE_PATH-y = -I $(ROOT_PATH)/../inc -I $(ROOT_PATH)/../mock/
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
.PHONY: objs clean test ut check_path

all: ut

check_path:
	-mkdir $(DIST_PATH)

%.o: %.c
	$(CXX) -c $(CXXFLAGS) $< -o $(DIST_PATH)/$(notdir $@)

objs: $(OBJS)

clean:
	-rm -rf $(DIST_PATH)
	-rm -rf $(ROOT_PATH)/ut

ut: check_path $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(ROOT_PATH)/ut $(TARGET_OBJS) -L $(LIBS_PATH) $(LDFLAGS) $(TEST_LDFLAGS)