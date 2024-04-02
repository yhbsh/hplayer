all: ffprobe opengl hplayer

hplayer: hplayer.c
	cc -g3 hplayer.c -o hplayer -lavcodec -lavformat -lswscale -lsdl2

opengl: opengl.c
	cc -g3 opengl.c -o opengl -lglfw3 -lglew -lavcodec -lavformat -lswscale -framework opengl -framework iokit -framework cocoa

ffprobe: ffprobe.c
	cc -g3 ffprobe.c -o ffprobe -lavcodec -lavformat
