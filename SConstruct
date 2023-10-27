# Define variables
src_dirs = ['src']
include_dirs = ['include']
lib_dir = 'lib'

# Define compiler and flags
env = Environment(CPPPATH = src_dirs + include_dirs)
env.Append(CCFLAGS = ['-Wall', '-ggdb'])
env.Append(LIBPATH = [lib_dir])
env.Append(LIBS = [File('lib/libraylib.a'), 'm'])  # Add your libraries here

# Get a list of all C files in the source directories

#c_files = [f for f in Glob(sdir + '/*.c') for sdir in src_dirs]  WHY the fuck does this not work?
c_files = []
for sdir in src_dirs:
    for f in Glob(sdir + '/*.cpp'):
        c_files.append(f)

# Compile C files
objs = env.Object(c_files)

# Link the object files
env.Program('main', objs)
