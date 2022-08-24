# discord-bot-image-manipulator

This Discord Bot, using discord.js, is able to take user images and do various
manipulations. Uses libjpeg and libpng C libraries for manipulation. The 
manipulation in question are image kernels for mainly comedic effect.
This bot would not have been possible without the help of Khan Winter
(https://github.com/thecoolwinter).

WARNING: This program only works with libpng 1.6, some packages only use 1.5 
which will not work properly.

INSTALLATION: You need to have gcc, Node.js, and npm.

Make - The make commands are as following:

make init: This installs 2 nessasary Node.js packages, axios, and discord.js.
This also compiles jpegFilter.c and creates the config files and initializes
Node.js.

make filter: This compiles jpegFilter.c.

make run: This starts the bot.


The commands for the bot are as follows:

%help - This displays the commands


%sharpen - We display the kernel,
kernel:
[  0 -2  0 ]
[ -2  9 -2 ]
[  0 -2  0 ]

This highlights borders as if the middle subpixel is "brighter" than the outer
subpixels, the resulting pixel will have a greater value.


%border - We display the kernel,
kernel:
[  0 -4  0 ]
[ -4 16 -4 ]
[  0 -4  0 ]

This will set pixels with similar "brightness" at or close to zero, and if the
differnce is great, will be very "bright". The effect shown is a large white 
outline of objects in the image.


%rot - This takes the image and sets it to quality zero in libjpeg which makes 
the resulting image very "blocky" and low on colors.


%text - This takes the image and creates a textfile where the resulting
characters correspond to that pixels "brightness".


PNG Support - Since all returned images are jpegs, we need to do a conversion 
from png to a data structure that jpeg can handle. We do this in a very 
low-effort way. Using libpng functions we convert a png with all of it's
different sorts of formats and return a png of format RGB888(a typical example
would be a ARGB -> RGB888, with no alpha layer). Then since we have a consistent
png of RGB888 we can give libjpeg a bytestream it can handle.
