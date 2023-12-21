# mock_config - Convert a .config file into a C autoconfig.h file

all: mock_config

# specify the compiler gcc here in case of cross-compiler
mock_config: mock_config.c config_define.c
	gcc -o mock_config mock_config.c config_define.c
clean:
	-rm -rf mock_config
