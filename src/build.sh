~/dev/ctime/ctime -begin gliewer_timings.ctm

# NOTE(hugo) : OPTIMIZED EXECUTABLE
#g++ -g -std=c++11 -O2 -Wno-write-strings -I ../dep/imgui/ -I ../dep/tinyobjloader/ -I ../dep/stb/ sdl_gl_layer.cpp -l SDL2 -l GL -l GLEW -D RIVTEN_SLOW=1 -D GLIEWER_DEBUG=1 -o ../build/gliewer.sh

# NOTE(hugo) : SLOW EXECUTABLE
g++ -g -std=c++11 -Wno-write-strings -I ../dep/imgui/ -I ../dep/tinyobjloader/ -I ../dep/stb/ sdl_gl_layer.cpp -l SDL2 -l GL -l GLEW -D RIVTEN_SLOW=1 -D GLIEWER_DEBUG=1 -o ../build/gliewer.sh
~/dev/ctime/ctime -end gliewer_timings.ctm
