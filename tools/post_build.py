# @title     StarBase
# @file      post_build.py
# @date      20240411
# @repo      https://github.com/ewowi/StarBase, submit changes to this file as PRs to ewowi/StarBase
# @Authors   https://github.com/ewowi/StarBase/commits/main
# @Copyright © 2024 Github StarBase Commit Authors
# @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
# @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com

Import('env')
import os
import shutil
import gzip

print("")
print("********************************************************************************************************************************")
print("* STARBASE IS LICENSED UNDER GPL-V3. BY INSTALLING STARBASE OR ONE IF ITS FORKS YOU IMPLICITLY ACCEPT THE TERMS AND CONDITIONS *")
print("********************************************************************************************************************************")
print("")

isGitHub = "runner" in os.path.expanduser("~") #do not copy in github PlatformIO CI (/home/runner/ is output dir in github)

if isGitHub:
    OUTPUT_DIR = "build_output{}".format(os.path.sep)
else:
    OUTPUT_DIR = "{}{}Downloads{}".format(os.path.expanduser("~"), os.path.sep, os.path.sep)


def _get_cpp_define_value(env, define):
    define_list = [item[-1] for item in env["CPPDEFINES"] if item[0] == define]
    if define_list:
        return define_list[0]
    return None

def _create_dirs(dirs=["firmware", "map"]):
    # check if output directories exist and create if necessary
    if not os.path.isdir(OUTPUT_DIR):
        os.mkdir(OUTPUT_DIR)
    for d in dirs:
        if not os.path.isdir("{}{}".format(OUTPUT_DIR, d)):
            os.mkdir("{}{}".format(OUTPUT_DIR, d))

def bin_rename_copy(source, target, env):
    app = _get_cpp_define_value(env, "APP")
    version = _get_cpp_define_value(env, "VERSION")
    pioenv = env["PIOENV"]

    if isGitHub:
        _create_dirs(["release"])
        # create string with location and file names based on pioenv
        bin_file = "{}release{}{}_{}_{}.bin".format(OUTPUT_DIR, os.path.sep, app, version, pioenv)
    else:
        bin_file = "{}{}_{}_{}.bin".format(OUTPUT_DIR, app, version, pioenv)

    # check if new target files exist and remove if necessary
    for f in [bin_file]: #map_file, 
        if os.path.isfile(f):
            os.remove(f)

    # copy firmware.bin to bin_file
    shutil.copy(str(target[0]), bin_file)
    print("  created " + bin_file)

env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", [bin_rename_copy])
