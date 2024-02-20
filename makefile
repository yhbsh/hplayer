all: opengl hplayer

hplayer: hplayer.c
	cc -g3 hplayer.c -o hplayer -lavcodec -lavformat -lswscale -lsdl2

opengl: opengl.c
	cc -g3 opengl.c -o opengl -lavcodec -lavformat -lswscale -lglfw -framework opengl
