import os

platform = "win"
#platform = "linux"

#build = "release"
build = "debug"

# Define variables
src_dirs = ['src', 'src/core']
include_dirs = ['include']

# Platform specific libraries
platform_libs = []


def get_all_files(dirs, ending):
    res = []
    for sdir in dirs:
        for f in Glob(sdir + '/*.' + ending):
            res.append(f)
    return res


def get_base_env():
    if platform == "win":
        env = Environment(
            CC = "gcc",
            ENV={'PATH': os.environ['PATH'], 'TEMP': os.environ['TEMP']},
            tools=['mingw']
        )
        lib_dir = 'lib/win'
    else:
        env = Environment(CC = "gcc")
        lib_dir = 'lib/linux'

    return env, lib_dir


def build_wren():
    env, lib_dir = get_base_env()

    wren_src_dirs = ['dependencies/wren/src/vm']
    wren_src_dirs.append('dependencies/wren/src/optional')
    wren_include_dirs = ['dependencies/wren/src/include']

    wren_src = get_all_files(wren_src_dirs, "c")
    env.Append(CPPPATH = wren_src_dirs + wren_include_dirs)
    env.StaticLibrary(os.path.join(lib_dir, 'wren'), wren_src)


def build_app():
    env, lib_dir = get_base_env()
    
    platform_libs = []
    if platform == "win":
        # For raylib specifically
        platform_libs = [
            'opengl32',
            'gdi32',
            'winmm',
        ]

    flags = []
    defines = []
    flags += ['-Wall', '-Wno-narrowing', '-Wno-sign-compare']  # Warnings we care about
    if build == 'debug':
        flags.append('-ggdb')
    elif build == 'release':
        defines.append('LOGGING_DISABLE')


    env.Append(CPPPATH = src_dirs + include_dirs)
    env.Append(CCFLAGS = flags)
    env.Append(LIBPATH = [lib_dir])
    env.Append(LIBS = [
        File(lib_dir + '/libraylib.a'), 
        File(lib_dir + '/libyaml.a'),
        File(lib_dir + '/libwren.a'),
        'm',
        *platform_libs
    ])
    env.Append(CPPDEFINES=defines)
    # Get a list of all C files in the source directories

    #c_files = [f for f in Glob(sdir + '/*.c') for sdir in src_dirs]  WHY the fuck does this not work?
    c_files = get_all_files(src_dirs, "cpp")

    # Compile C files
    objs = env.Object(c_files)

    # Link the object files
    env.Program('app', objs)


def main():
    build_wren()
    build_app()

main()