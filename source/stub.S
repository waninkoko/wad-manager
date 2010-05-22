	.rodata

	.align 4
	.global bgData
bgData:
	.incbin "../data/background"


	.align 4
	.global bgMusic
bgMusic:
	.incbin "../data/bgmusic"
bgMusic_End:

	.global bgMusic_Len
bgMusic_Len:
	.long bgMusic_End - bgMusic
