UNAME := $(shell uname)

ifeq ($(UNAME), Linux)
	LIBS = -lGL -lGLEW -lm -lglfw
	FRAMEWORK = 
	CFLAGS=-D LINUX
endif
ifeq ($(UNAME), Darwin)
	LIBS = -lm -lglfw -lglew
	FRAMEWORK = -framework OpenGL -framework Cocoa
	CFLAGS=-D RETINA
endif

all:
	g++ ${LIBS} ${FRAMEWORK} ${CFLAGS} *.cpp -o raytraycer
