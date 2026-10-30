

tut: tut.cpp fragment.original.glsl vertex.original.glsl
	clang++ -std=c++11 -stdlib=libc++ -lglfw -framework CoreVideo -framework OpenGL -framework IOKit -framework Cocoa -framework Carbon -o tut tut.cpp

copy:
	scp -r shader/ cube:

deploy: copy
	scp cpu-stats-gl.cpp as-light.py led.service homekit.service r8188eu.conf cube:
	ssh cube 'sudo mv r8188eu.conf /etc/modprobe.d/ && sudo systemctl enable ~/led.service ~/homekit.service'

	ssh cube g++ -g -o cpu-stats-gl cpu-stats-gl.cpp -std=c++11 -lbrcmEGL -lbrcmGLESv2 -I/opt/vc/include -L/opt/vc/lib -Lrpi-rgb-led-matrix/lib -lrgbmatrix -lrt -lm -lpthread -lstdc++ -Irpi-rgb-led-matrix/include/

# ponytail: one-time setup of a fresh Raspbian Buster Pi (needs /opt/vc brcm GL libs), then run `make deploy`.
# untested end to end, written from what the running cube has; reboot after.
setup:
	ssh cube 'sudo apt-get install -y git build-essential python3-pip'
	ssh cube '[ -d rpi-rgb-led-matrix ] || git clone https://github.com/hzeller/rpi-rgb-led-matrix && make -C rpi-rgb-led-matrix/lib'
	ssh cube 'sudo pip3 install HAP-python==4.9.2'
	ssh cube 'sudo mkdir -p /etc/cube && sudo ln -sfn /home/lad1337/shader /etc/cube/shader'
	ssh cube 'sudo ln -sf /home/lad1337/cpu-stats-gl /usr/bin/cpu-stats-gl && sudo ln -sf /home/lad1337/as-light.py /usr/bin/as-light.py'
	ssh cube 'grep -q isolcpus=3 /boot/cmdline.txt || sudo sed -i "s/$$/ isolcpus=3/" /boot/cmdline.txt'
	ssh cube 'sudo sed -i "s/^dtparam=audio=on/dtparam=audio=off/" /boot/config.txt'
