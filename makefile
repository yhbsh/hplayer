hplayer: hplayer.c
	cc -g3 hplayer.c -o hplayer -lavcodec -lavformat -lswscale -lsdl2
