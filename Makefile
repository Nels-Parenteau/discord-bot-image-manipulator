init:
	npm init -y
	npm install discord.js@13.9.2
	npm install axios
	gcc -g -o jpegFilter jpegFilter.c -ljpeg -lpng
	echo -e '{\n  "token": "YOUR TOKEN HERE"\n}' >> config.json

filter:
	gcc -g -o jpegFilter jpegFilter.c -ljpeg -lpng

run:
	node index.js