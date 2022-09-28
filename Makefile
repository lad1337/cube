

tut: tut.cpp fragment.original.glsl vertex.original.glsl
	clang++ -std=c++11 -stdlib=libc++ -lglfw -framework CoreVideo -framework OpenGL -framework IOKit -framework Cocoa -framework Carbon -o tut tut.cpp

copy:
	scp cpu-stats-gl.cpp cube:
	scp vertex.* cube:
	scp fragment.template.glsl cube:
	scp render.*.glsl cube:
	scp as-light.py cube:
	scp install.sh cube:

init:
	scp rgb-matrix.sh cube:
	scp init.sh cube:
	ssh cube sudo bash ./init.sh

after:
	scp *.service cube:
	ssh cube sudo systemctl daemon-reload
	ssh cube sudo systemctl enable homekit led
	ssh cube sudo systemctl start led homekit

build: copy
	ssh cube g++ -g -o cpu-stats-gl cpu-stats-gl.cpp -std=c++11 -lbrcmEGL -lbrcmGLESv2 -I/opt/vc/include -L/opt/vc/lib -Lrpi-rgb-led-matrix/lib -lrgbmatrix -lrt -lm -lpthread -lstdc++ -Irpi-rgb-led-matrix/include/
	ssh cube sudo bash ./install.sh


all: pre build after
