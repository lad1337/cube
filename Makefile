
hello:
	g++ /System/Library/Frameworks/GLUT.framework/GLUT /System/Library/Frameworks/OpenGL.framework/OpenGL hello_world.cpp -o demo -Wdeprecated-declarations

cube_:
	g++ /System/Library/Frameworks/GLUT.framework/GLUT /System/Library/Frameworks/OpenGL.framework/OpenGL cube-demo.cpp -o demo -Wdeprecated-declarations

cube:
	clang++ -std=c++11 -stdlib=libc++ -lglfw -framework CoreVideo -framework OpenGL -framework IOKit -framework Cocoa -framework Carbon -o demo cube-demo.cpp

desk: desktop-copy.cpp
	clang++ -std=c++11 -stdlib=libc++ -lglfw -framework CoreVideo -framework OpenGL -framework IOKit -framework Cocoa -framework Carbon -o desk desktop-copy.cpp

tut: tut.cpp fragment.original.glsl vertex.original.glsl
	clang++ -std=c++11 -stdlib=libc++ -lglfw -framework CoreVideo -framework OpenGL -framework IOKit -framework Cocoa -framework Carbon -o tut tut.cpp

glfw:
	clang++ -std=c++11 -stdlib=libc++ -lglfw -framework CoreVideo -framework OpenGL -framework IOKit -framework Cocoa -framework Carbon -o test-glfw test-glfw.cpp 

deploy: 
	scp cpu-stats-gl.cpp cube:
	scp fragment.theroutamod.glsl cube:
	scp vertex.original.glsl cube:

	ssh cube g++ -g -o cpu-stats-gl cpu-stats-gl.cpp -std=c++11 -lbrcmEGL -lbrcmGLESv2 -I/opt/vc/include -L/opt/vc/lib -Lrpi-rgb-led-matrix/lib -lrgbmatrix -lrt -lm -lpthread -lstdc++ -Irpi-rgb-led-matrix/include/
