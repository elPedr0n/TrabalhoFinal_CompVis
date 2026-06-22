SRC = src/main.cpp src/glad.c src/textrendering.cpp src/stb_image.cpp src/stb_image_write.cpp src/tiny_obj_loader.cpp src/tiny_gltf.cpp src/player.cpp src/buildtriangles.cpp src/animation.cpp src/projectiles.cpp src/particles.cpp src/enemies.cpp src/collectibles.cpp src/correcao.cpp src/gltf_utils.cpp src/screens.cpp src/breakables.cpp src/fragments.cpp src/gamepad.cpp

./bin/Linux/main: $(SRC) include/* CMakeLists.txt CMakePresets.json
	cmake --preset default-config
	cmake --build --preset default-build

.PHONY: clean run
clean:
	rm -f bin/Linux/main

run: ./bin/Linux/main
	cd bin/Linux && ./main
