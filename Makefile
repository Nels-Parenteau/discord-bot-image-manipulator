init:
	npm init -y
	npm install discord.js@13.9.2
	npm install axios
	gcc -o jpegFilter jpegFilter.c -ljpeg

filter:
	gcc -o jpegFilter jpegFilter.c -ljpeg

run:
	node index.js