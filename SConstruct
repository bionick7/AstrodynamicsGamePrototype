import os

build_tests = False
platform = "win"

# Define variables
src_dirs = ['src']
include_dirs = ['include']
lib_dir = 'lib'

# Platform specific libraries
platform_libs = []

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

    defines = []
    if build_tests:
        defines.append('RUN_TESTS')

    env.Append(CPPPATH = src_dirs + include_dirs)
    env.Append(CCFLAGS = ['-Wall', '-Wno-narrowing', '-ggdb'])
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
    c_files = []
    for sdir in src_dirs:
        for f in Glob(sdir + '/*.cpp'):
            c_files.append(f)

    # Compile C files
    objs = env.Object(c_files)

    # Link the object files
    env.Program('app', objs)

main()