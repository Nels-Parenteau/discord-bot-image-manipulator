init:
	npm init -y
	npm install discord.js
	gcc -o jpegFilter jpegFilter.c -ljpeg

filter:
	gcc -o jpegFilter jpegFilter.c -ljpeg