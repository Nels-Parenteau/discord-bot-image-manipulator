init:
	npm init -y
	npm install discord.js@13.9.2
	npm install axios
	gcc -g -o jpegFilter jpegFilter.c -ljpeg

filter:
	gcc -g -o jpegFilter jpegFilter.c -ljpeg

run:
	node index.js