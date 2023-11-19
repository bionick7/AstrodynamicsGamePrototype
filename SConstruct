import os

#platform = "win"
platform = "linux"

#build = "release"
build = "debug"

# Define variables
src_dirs = ['src']
include_dirs = ['include']
lib_dir = 'lib'

# Platform specific libraries
platform_libs = []

def get_all_cpp_files(dirs):
    res = []
    for sdir in dirs:
        for f in Glob(sdir + '/*.cpp'):
            res.append(f)
    return res

def main():
    if platform == "win":
        env = Environment(
            CC = "gcc",
            ENV={'PATH': os.environ['PATH'], 'TEMP': os.environ['TEMP']},
            tools=['mingw']
        )
        lib_dir = 'lib/win'
        platform_libs = [
            'opengl32',
            'gdi32',
            'winmm',
        ]
    else:
        env = Environment(CC = "gcc")
        lib_dir = 'lib/linux'
        platform_libs = []

    flags = []
    defines = []
    flags += ['-Wall', '-Wno-narrowing', '-Wno-sign-compare']  # Warnings we care about
    if build == "debug":
        flags.append("-ggdb")
    elif build == "release":
        defines.append('LOGGING_DISABLE')

    env.Append(CPPPATH = src_dirs + include_dirs)
    env.Append(CCFLAGS = flags)
    env.Append(LIBPATH = [lib_dir])
    env.Append(LIBS = [
        File(lib_dir + '/libraylib.a'), 
        File(lib_dir + '/libyaml.a'),
        'm',
        *platform_libs
    ])
    env.Append(CPPDEFINES=defines)
    # Get a list of all C files in the source directories

    #c_files = [f for f in Glob(sdir + '/*.c') for sdir in src_dirs]  WHY the fuck does this not work?
    c_files = get_all_cpp_files(src_dirs)

    # Compile C files
    objs = env.Object(c_files)

    # Link the object files
    env.Program('app', objs)

main()