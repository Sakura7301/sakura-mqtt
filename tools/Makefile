# mock_config - Convert a .config file into a C autoconfig.h file
#PATH
ROOT_PATH = $(shell pwd)
MOCK_CONFIG_PATH = $(ROOT_PATH)/mock_config
all: mock_config

# specify the compiler gcc here in case of cross-compiler
mock_config: mock_config.c config_define.c
	@echo "Compiling $<"
	@gcc -o mock_config mock_config.c config_define.c
	@echo "Creating mock_config: $(MOCK_CONFIG_PATH)"
clean:
	-rm -rf mock_config
