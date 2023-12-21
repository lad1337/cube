

tut: tut.cpp fragment.original.glsl vertex.original.glsl
	clang++ -std=c++11 -stdlib=libc++ -lglfw -framework CoreVideo -framework OpenGL -framework IOKit -framework Cocoa -framework Carbon -o tut tut.cpp

copy:
	scp -r shader/ cube:

deploy: copy
	scp cpu-stats-gl.cpp cube:

	ssh cube g++ -g -o cpu-stats-gl cpu-stats-gl.cpp -std=c++11 -lbrcmEGL -lbrcmGLESv2 -I/opt/vc/include -L/opt/vc/lib -Lrpi-rgb-led-matrix/lib -lrgbmatrix -lrt -lm -lpthread -lstdc++ -Irpi-rgb-led-matrix/include/
