import re
import os
import sys
# import oss2
import json
import shutil
import zipfile
import platform
import datetime



##################
# Common Configs #
##################
PWD                 = os.getcwd()
SYSTEM              = ''
RELEASE_PATH        = os.path.join(PWD, 'release')
LIBS_PATH           = os.path.join(RELEASE_PATH, 'libs')
DIST_PATH           = os.path.join(PWD, 'dist')
SDK_VERSION_FILE    = os.path.join(PWD, 'inc/sakura_mqtt_client.h')
SDK_VERSION         = ''
BUILD_TIME          = datetime.datetime.now().strftime('%Y%m%d')
SDK_PACKAGE_PREFIX  = 'libsakura-mqtt-sdk'
CONFIGS_JSON_FILE   = os.path.join(PWD, 'configs.json')
TEMPLATES_JSON_FILE = os.path.join(PWD, 'templates.json')
DOT_CONFIG_FILE     = os.path.join(PWD, '.config')
AUTOCONFIG_HEADER   = os.path.join(PWD, 'inc/sakura_autoconfig.h')

# return version string
def get_sdk_version():
    f = open(SDK_VERSION_FILE)
    for line in f.readlines():
        # print('line: ' + line)
        # example: '#define VERSION "v1.2.6"'
        if line.find('#define SAKURA_MQTT_SDK_VERSION ') != -1:
            f.close()
            return line.split('"')[1]
    f.close()
    return ''

def zip_compress(source_dir, output_filename):
    zipf = zipfile.ZipFile(output_filename, 'w', zipfile.ZIP_DEFLATED)
    pre_len = len(os.path.dirname(source_dir))
    for parent, dirnames, filenames in os.walk(source_dir):
        for filename in filenames:
            path_file = os.path.join(parent, filename)
            arcname = path_file[pre_len:].strip(os.path.sep)
            zipf.write(path_file, arcname)
    zipf.close()

def get_archive_config(archive):
    doc = []
    include = []
    demo = []
    t_archive = templates['archive']

    if 'doc' in archive:
        for d in archive['doc']:
            for dd in t_archive['doc'][d]:
                doc.append(dd)

    if 'include' in archive:
        for i in archive['include']:
            for ii in t_archive['include'][i]:
                include.append(ii)

    if 'demo' in archive:
        for d in archive['demo']:
            for dd in t_archive['demo'][d]:
                demo.append(dd)

    return doc, include, demo

def get_archive_external_lib_config(archive):
    external_lib = []
    t_archive = templates['archive']
    if 'external_lib' in archive and 'external_lib' in t_archive:
        for d in archive['external_lib']:
            if d in t_archive['external_lib']:
                for dd in t_archive['external_lib'][d]:
                    external_lib.append(dd)
    return external_lib

# write file declaration for sdk header
def write_file_declaration(output_file,out_desc):
    declaration = '/**\n\
 * Copyright (c) 2022-2023 SAKURA. All rights reserved.\n\
 *\n\
 * @file {}\n\
 * @brief {}\n\
 * @version 1.0.0\n\
 * @author Sakura\n\
 * @date   2023-12-20\n\
 *\n\
 * CHANGELOG:\n\
 * DATE             AUTHOR          REASON\n\
 * 2023-12-20       Sakura          Init version;\n\
 */\n'.format(os.path.basename(output_file), out_desc)

    return declaration

def begin_generate_release_headers(output_file, headers, out_desc):
    if os.path.exists(output_file):
        os.remove(output_file)
    out = open(output_file, 'a+')
    paths = output_file.split('/')
    file_name = paths[len(paths) - 1]
    file_name_prefix = file_name.split('.')[0]
    macro = file_name_prefix.upper() + '_H__'
    ##out.write('/*\n * Copyright (c) 2021-2023 SAKURA. All rights reserved.\n */\n\n')
    file_declaration = write_file_declaration(output_file, out_desc)
    out.write(file_declaration)
    out.write('#ifndef ' + macro + '\n')
    out.write('#define ' + macro + '\n\n')
    out.write('#ifdef __cplusplus\nextern "C" {\n#endif\n\n')

    if headers and len(headers) > 0:
        for line in headers:
            out.write(line + '\n')

    out.close()

def finish_generate_release_headers(output_file):
    out = open(output_file, 'a+')
    out.write('\n#ifdef __cplusplus\n}\n#endif\n\n')
    out.write('#endif\n')
    out.close()

def push(stack, item):
    stack.append(item)

def pop(stack):
    return stack.pop()

def is_empty(stack):
    return len(stack) == 0

def get_top(stack):
    return stack[len(stack) - 1]

def copy_and_remove_macros(macro_configs, input_file, output_file, begin_str, end_str, only_copy):
    state = None
    pre_process_stack = []
    if not os.path.exists(input_file):
        print (input_file + ' not exists.')
        return -1
    if begin_str is None or begin_str == '':
        state = 'W'
    out = open(output_file, 'a+')
    for line in open(input_file):
        # print line
        if state is None and line.find(begin_str) >= 0:
            state = 'N' # N = begin writing on Next line, skip this line
        elif state is not None and state != 'S' and end_str is not None and line.find(end_str) >= 0:
            state = 'S' # S = Stop writing
        elif only_copy is False:
            if state is not None and line.find('#ifdef') >= 0:
                # if state is 'P', it means we are processing useless codes, just push 'P' to stack
                if state == 'P': # P = Pause writing, we are processing useless codes eliminated by a macro
                    push(pre_process_stack, 'P')
                else:
                    macro = line.strip().split(' ')[1].replace('\n', '')
                    if macro in macro_configs and macro_configs[macro] != 'n':
                        state = 'N'
                        push(pre_process_stack, 'N')
                    else:
                        state = 'P'
                        push(pre_process_stack, 'P')
            elif state is not None and line.find('#ifndef') >= 0:
                # if state is 'P', it means we are processing useless codes, just push 'P' to stack
                if state == 'P':
                    push(pre_process_stack, 'P')
                else:
                    macro = line.strip().split(' ')[1].replace('\n', '')
                    if macro in macro_configs and macro_configs[macro] != 'n':
                        state = 'P'
                        push(pre_process_stack, 'P')
                    else:
                        state = 'N'
                        push(pre_process_stack, 'N')
            elif state is not None and line.find('#if') >= 0:
                # if state is 'P', it means we are processing useless codes, just push 'P' to stack
                if state == 'P': # P = Pause writing, we are processing useless codes eliminated by a macro
                    push(pre_process_stack, 'P')
                else:
                    if 'defined' in line: # define(MACRO)
                        values = []
                        macros = line.strip().split('(')
                        for key in macros:
                            if ')' in key:
                                macro = key.strip().split(')')[0]
                                if macro in macro_configs and macro_configs[macro] != 'n':
                                    #state = 'N'
                                    #push(pre_process_stack, 'N')
                                    push(values, 1)
                                else:
                                    #state = 'P'
                                    #push(pre_process_stack, 'P')
                                    push(values, 0)
                        value = 0
                        if '&&' in line:
                            value = 1
                            for v in values:
                                value = value * v # &&
                        elif '||' in line:
                            value = 0
                            for v in values:
                                value = value + v # ||
                        else:
                            value = values[0]

                        ##
                        if value > 0:
                            state = 'N'
                            push(pre_process_stack, 'N')
                        else:
                            state = 'P'
                            push(pre_process_stack, 'P')
                    else:
                        macro = line.strip().split(' ')[1].replace('\n', '')
                        ## handle #if 0 case
                        if macro == "0":
                            state = 'P'
                            push(pre_process_stack, 'P')
                        else:
                            value = line.strip().split('==')[1].replace('\n', '')
                            if macro in macro_configs and macro_configs[macro] != value:
                                state = 'N'
                                push(pre_process_stack, 'N')
                            else:
                                state = 'P'
                                push(pre_process_stack, 'P')

            elif state is not None and line.find('#else') >= 0:
                # pop stack and set the value to state
                state = pop(pre_process_stack)
                top = 'N'
                # check top stack value, if it's 'P', just set state to 'P' and push another 'P' to stack
                if not is_empty(pre_process_stack):
                    top = get_top(pre_process_stack)
                if top == 'P':
                    state = 'P'
                    push(pre_process_stack, 'P')
                else:
                    if state == 'N':
                        state = 'P'
                        push(pre_process_stack, 'P')
                    else:
                        state = 'N'
                        push(pre_process_stack, 'P')
            elif state is not None and line.find('#endif') >= 0:
                pop(pre_process_stack)
                if not is_empty(pre_process_stack):
                    # the stack is not empty, set the top stack value to state
                    state = get_top(pre_process_stack)
                else:
                    # the stack is empty, restore 'Write' on the next line
                    state = 'N'
        # print state
        if state == 'W':# Write this line
            out.write(line)
        elif state == 'N':
            state = 'W'
        elif state == 'S':
            out.close
            return 0
    return 0

def insert_release_headers(macro_configs, input_file, output_file, begin_str, end_str, only_copy):
    return copy_and_remove_macros(macro_configs, input_file, output_file, begin_str, end_str, only_copy)

def generate_archive_headers(config, macro_configs):
    doc, includes, demo = get_archive_config(config['archive'])
    for conf in includes:
        out_path = conf['out']
        out_desc = conf['desc']
        override = True
        if 'override' in conf:
            override = conf['override']
        if os.path.exists(out_path) and not override:
            continue
        print ('generate header file:' + out_path)
        if 'copy' in conf:
            if os.path.exists(out_path):
                os.remove(out_path)
            shutil.copy(conf['copy'], out_path)
            continue
        in_files = conf['in']
        headers = None
        if 'headers' in conf:
            headers = conf['headers']
        begin_generate_release_headers(out_path, headers, out_desc)
        for in_file in in_files:
            file_path = in_file['file']
            if 'without' in in_file:
                ignore = True
                without = in_file['without']
                # use for-else to figure out whether we should ignore the header file.
                for item in without:
                    if item in macro_configs and macro_configs[item] == 'y':
                        break
                else:
                    ignore = False
                if ignore is True:
                    continue
            if 'with' in in_file:
                ignore = True
                for item in in_file['with']:
                    if item in macro_configs and macro_configs[item] == 'y':
                        ignore = False
                        break
                if ignore is True:
                    continue
            if 'begin' in in_file:
                begin = in_file['begin']
                end = in_file['end']
                if 'only_copy' in in_file:
                    insert_release_headers(macro_configs, file_path, out_path, begin, end, True)
                else:
                    insert_release_headers(macro_configs, file_path, out_path, begin, end, False)
        finish_generate_release_headers(out_path)

def copy_demo_file(in_file, out_dir, macro_configs):
    (filepath, filename) = os.path.split(in_file)
    out_file = os.path.join(out_dir, filename)
    copy_and_remove_macros(macro_configs, in_file, out_file, None, None, True)

def archive_release_files(release_name, archive_libs_path, archive, macro_configs):
    dst_pkg = os.path.join(PWD, release_name + '.package')
    release_name_prefix = os.path.join(PWD, 'release', release_name + '.package')
    release_package_name = release_name_prefix + '.zip'
    md5_file_name = release_name_prefix + '.md5'

    doc, include, demo = get_archive_config(archive)

    lib_path = os.path.join(dst_pkg, 'lib')
    include_path = os.path.join(dst_pkg, 'include')
    doc_path = os.path.join(dst_pkg, 'doc')
    demo_path = os.path.join(dst_pkg, 'demo')

    # copy the raw package
    if os.path.exists(dst_pkg):
        shutil.rmtree(dst_pkg, True)

    os.mkdir(dst_pkg)
    os.mkdir(include_path)
    os.mkdir(doc_path)
    os.mkdir(demo_path)

    if os.path.exists(archive_libs_path):
        if os.path.exists(lib_path):
            shutil.rmtree(lib_path)
        shutil.copytree(archive_libs_path, lib_path)

    for file in include:
        abs_file = os.path.join(PWD, file['out'])
        shutil.copy(abs_file, include_path)

    for file in doc:
        abs_file = os.path.join(PWD, file)
        shutil.copy(abs_file, doc_path)

    for file in demo:
        abs_file = os.path.join(PWD, file)
        copy_demo_file(abs_file, demo_path, macro_configs)

    if os.path.exists(DOT_CONFIG_FILE):
        shutil.copy(DOT_CONFIG_FILE, dst_pkg)

    # generate .zip package
    zip_compress(dst_pkg, release_package_name)

    # remove the temp folder
    shutil.rmtree(dst_pkg, True)

def make_it(cmd):
    print('==================================================')
    print(cmd)
    print('==================================================')

    # clean build path
    shutil.rmtree(DIST_PATH, True)
    if os.path.exists(DIST_PATH) == False:
        print('mkdir ' + DIST_PATH)
        os.mkdir(DIST_PATH)

    if os.system(cmd):
        return -1
    return 0

def build_with_make(config):
    if 'customer' in config:
        customer = config['customer']
    else:
        customer = 'unknown'

    compiler = None
    cross_compile = None
    ssl_version = None
    if 'compiler' in config:
        compiler = config['compiler']
    if 'cross_compile' in config:
        cross_compile = config['cross_compile']
    if 'ssl_version' in config:
        ssl_version = config['ssl_version']

    release_name = SDK_PACKAGE_PREFIX + '-' + SDK_VERSION + '-' + BUILD_TIME + '-' + customer

    if 'platform' in config:
        release_name += '-' + config['platform']
    if 'feature' in config:
        release_name += '-' + config['feature']

    extra_cflags = ''
    extra_ldflags = ''

    if 'macros' in config:
        for macro in config['macros']:
            extra_cflags += ' -D ' + macro

    if 'cflags' in config:
        for cflag in config['cflags']:
            extra_cflags += ' ' + cflag

    if 'ldflags' in config:
        for ldflag in config['ldflags']:
            extra_ldflags += ' ' + ldflag
    print('==================================================')
    print(extra_cflags)
    print(extra_ldflags)
    print('==================================================')

    macro_configs = {}
    if '.config' in config:
        shutil.copy(os.path.join(PWD, 'config', config['.config']), DOT_CONFIG_FILE)
        if os.path.exists(DOT_CONFIG_FILE):
            parse_macro_config(DOT_CONFIG_FILE, macro_configs)
            gen_autoconfig_header_file(AUTOCONFIG_HEADER, macro_configs)

    # compile command
    compile_cmd  = 'make lib PYTHON_BUILD=y'
    if cross_compile:
        compile_cmd += ' CROSS_COMPILE='        + cross_compile
    if compiler:
        compile_cmd += ' COMPILER='             + compiler
    if extra_cflags:
        compile_cmd += ' EXTRA_CFLAGS=\"'       + extra_cflags + '\"'
    if extra_ldflags:
        compile_cmd += ' EXTRA_LDFLAGS=\"'      + extra_ldflags + '\"'
    if 'environments' in config:
        for e in config['environments']:
            compile_cmd += ' ' + e

    serv_config_list = []
    if "server" in config:
        if isinstance(config["server"], list) == True:
            serv_config_list = config["server"]
        elif isinstance(config["server"], str) == True:
            serv_config_list = [config["server"]]
    # by default: production environment
    if len(serv_config_list) == 0:
        serv_config_list = ["production"]

    release_config_list = ["release", "debug"]

    static_lib_postfix = '.a'
    if compiler == 'KEIL':
        static_lib_postfix = '.lib'

    for serv in serv_config_list:
        shutil.rmtree(LIBS_PATH, True)
        if os.path.exists(LIBS_PATH) == False:
            print('mkdir ' + LIBS_PATH)
            os.mkdir(LIBS_PATH)
        release_name += '-' + serv
        for release in release_config_list:
            release_lib_path = os.path.join(LIBS_PATH, release)
            release_prefix = release_name + '-' + release
            static_lib  = os.path.join(DIST_PATH, release_prefix + static_lib_postfix)
            dynamic_lib = os.path.join(DIST_PATH, release_prefix + '.so')

            make_cmd  = compile_cmd
            make_cmd += ' RELEASE_PREFIX=' + release_prefix
            make_cmd += ' ENV=' + serv.upper()
            make_cmd += ' RELEASE=' + release.upper()

            if make_it(make_cmd) == -1:
                return -1

            if os.path.exists(release_lib_path) == False:
                print('mkdir ' + release_lib_path)
                os.mkdir(release_lib_path)

            # copy libraries if they exist
            for lib in [static_lib, dynamic_lib]:
                if os.path.exists(lib):
                    shutil.copy(lib, release_lib_path)

            # copy external lib if they exist
            external_libs = get_archive_external_lib_config(config['archive'])
            for lib in external_libs:
                if os.path.exists(lib):
                    shutil.copy(lib, release_lib_path)
        generate_archive_headers(config, macro_configs)
        archive_release_files(release_name, LIBS_PATH, config['archive'], macro_configs)
    return 0

def build_foreach(configs):
    for config in configs:
        # enable or disable
        if "enable" in config and config['enable'] == False:
            continue
        if build_with_make(config) != 0:
            return -1
    return 0

# build entry
def multiple_platform_build():
    ret = 0
    global SYSTEM
    SYSTEM = platform.system()
    global SDK_VERSION
    SDK_VERSION = get_sdk_version()
    print('Build environment:\nsystem: {}\nversion: {}\ntime: {}\n'.format(SYSTEM, SDK_VERSION, BUILD_TIME))
    os.system('mkdir archive/include')

    # clean release path
    shutil.rmtree(RELEASE_PATH, True)
    if os.path.exists(RELEASE_PATH) == False:
        print('mkdir ' + RELEASE_PATH)
        os.mkdir(RELEASE_PATH)

    if SYSTEM == 'Linux':
        if 'linux' in configs:
            ret = build_foreach(configs['linux'])
    else:
        print('Not support system!')
        ret = -1

    if ret != 0:
        print('Build failed, exit!')
    else:
        print("Build finished!")

    return ret

def parse_macro(line):
    key = None
    value = None

    matches = line.partition('=')
    if matches[1] == '=' and matches[2] != '':
        key = matches[0].strip()
        value = matches[2].partition('#')[0].strip()

    return key, value

def parse_macro_config(filename, config):
    f = open(filename, 'r')
    for line in f.readlines():
        line = line.strip()
        if len(line) > 0 and line[0] == '#':
            continue
        key, value = parse_macro(line)
        if key is not None and value is not None:
            # set key & value
            config[key] = value
    f.close()

def gen_autoconfig_header_file(filename, macro_configs):
    f = open(filename, 'w')
    f.write('''/*
 * Copyright (c) 2021-2023 SAKURA. All rights reserved.
 * sakura_autoconfig.h -- Autogenerated! Do not edit.
 */

#ifndef SAKURA_AUTOCONFIG_H__
#define SAKURA_AUTOCONFIG_H__

''')

    for key in macro_configs:
        value = macro_configs[key]
        if key is not None and value is not None:
            if value == 'y':
                strline = '#define ' + key + ' 1'
            elif value == 'n':
                strline = '#undef ' + key
            else:
                strline = '#define ' + key + ' ' + value

            f.write(strline + '\n')

    f.write('\n#endif /* SAKURA_AUTOCONFIG_H__ */\n\n')
    f.close()

def load_json_config():
    global configs
    f = open(CONFIGS_JSON_FILE, 'rb')
    configs = json.loads(f.read(), encoding='utf8')
    f.close()

    global templates
    f = open(TEMPLATES_JSON_FILE, 'rb')
    templates = json.loads(f.read(), encoding='utf8')
    f.close()


# entry
if __name__ == '__main__':
    load_json_config()
    ret = multiple_platform_build()
    # failed with return code non-zero
    if ret != 0:
        sys.exit(1)

    sys.exit(0)
