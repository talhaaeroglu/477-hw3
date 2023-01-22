all:
	g++ main.cpp -g -o main \
        `pkg-config --cflags --libs freetype2` \
        -lglfw -lGLU -lGL -lGLEW -I/home/talha/glm-0.9.9.8/ -I/usr/include/freetype2 -L/usr/local/lib -lfreetype
