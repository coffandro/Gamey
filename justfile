build_dir := "build"

setup:
    cmake -B {{build_dir}} -S .

build: setup
    cmake --build {{build_dir}}

run: build
    ./{{build_dir}}/src/gamey

clean:
    rm -rf {{build_dir}}
