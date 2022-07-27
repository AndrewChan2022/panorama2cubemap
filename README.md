# panorama2cubemap

This project has been tested on:

macOS 12.4 Monterey

XCode 13.4.1


code from https://stackoverflow.com/a/34720686/2482283

other implement:

c++ without dependency:  https://github.com/chinhsuanwu/360-converter

python: https://github.com/sunset1995/py360convert


imagemagick script: http://www.fmwconcepts.com/imagemagick/sphericalpano2cube/index.php


## online test tools

https://shakthijuego.github.io/skybox-viewer/dist/index.html

## interface

    bool pano2cube(std::string inpath, std::vector<std::string>& outpaths, int width = -1, std::string mergedPath = "");
    

outpaths 6 face order: +X, -X, +Y, -Y, +Z, -Z

    +X right
    -X left
    +Y top 
    -Y bottom
    +Z front
    -Z back
    
correspond to OpenGL define:

    GL_TEXTURE_CUBE_MAP_POSITIVE_X
    GL_TEXTURE_CUBE_MAP_NEGATIVE_X
    GL_TEXTURE_CUBE_MAP_POSITIVE_Y
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Y
    GL_TEXTURE_CUBE_MAP_POSITIVE_Z
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Z 
    
## build test code

require install opencv first, for macos:
    
    brew install opencv

build, the test code at the tail of the cpp file

    mkdir build && cd build
    cmake ..
    make -j12

run command:

    ./pano2cube ../src/auto_stop_spherical_pano.jpg

generate:

    ../src/auto_stop_spherical_pano0.png
    ../src/auto_stop_spherical_pano1.png
    ../src/auto_stop_spherical_pano2.png
    ../src/auto_stop_spherical_pano3.png
    ../src/auto_stop_spherical_pano4.png
    ../src/auto_stop_spherical_pano5.png






## test result

there is test code in the file, read auto_stop_spherical_pano.jpg and generate 6 faces.


test image:

<img src="src/auto_stop_spherical_pano.jpg"/>

generated image is:

    src/auto_stop_spherical_pano0.png
    src/auto_stop_spherical_pano1.png
    src/auto_stop_spherical_pano2.png
    src/auto_stop_spherical_pano3.png
    src/auto_stop_spherical_pano4.png
    src/auto_stop_spherical_pano5.png

a correct result can be found in:

    src/correct_result/auto_stop_spherical_pano0.png
    src/correct_result/auto_stop_spherical_pano1.png
    src/correct_result/auto_stop_spherical_pano2.png
    src/correct_result/auto_stop_spherical_pano3.png
    src/correct_result/auto_stop_spherical_pano4.png
    src/correct_result/auto_stop_spherical_pano5.png


left

<img src="src/correct_result/auto_stop_spherical_pano1.png"/>

front

<img src="src/correct_result/auto_stop_spherical_pano4.png"/>

right

<img src="src/correct_result/auto_stop_spherical_pano0.png"/>

back

<img src="src/correct_result/auto_stop_spherical_pano5.png"/>

top

<img src="src/correct_result/auto_stop_spherical_pano2.png"/>

bottom

<img src="src/correct_result/auto_stop_spherical_pano3.png"/>

merged 

<img src="src/correct_result/auto_stop_spherical_pano6.png"/>

