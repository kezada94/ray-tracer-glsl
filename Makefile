UNAME := $(shell uname)

ifeq ($(UNAME), Linux)
	LIBS = -lm -lglfw -lglew -lGl
	FRAMEWORK = 
endif
ifeq ($(UNAME), Darwin)
	LIBS = -lm -lglfw -lglew
	FRAMEWORK = -framework OpenGL -framework Cocoa
endif

all:
	g++ ${LIBS} ${FRAMEWORK} *.cpp -o raytraycer