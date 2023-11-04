#gcc src/global_state.c src/main.c -Llib -l:libraylib.a -lm -Iinclude -Isrc -o main
g++ -o app src/*.cpp -g -Wall -Wno-narrowing -rdynamic -Llib -l:libraylib.a -l:libyaml-0.so.2 -lm -Iinclude -Isrc > build_outp.txt
