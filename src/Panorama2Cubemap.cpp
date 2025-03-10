//
// pano2cube (c) 2022-2025 by Emanuel Jobstl, AndrewChan2022, and Contributors
//
// pano2cube is licensed under a
// Creative Commons Attribution-ShareAlike 3.0 Unported License.
//
// You should have received a copy of the license along with this
// work.  If not, see <http://creativecommons.org/licenses/by-sa/3.0/>.
//

#include "Panorama2Cubemap.hpp"
#include <iostream>
#include <ctime>
#include <fstream>

#include "opencv2/highgui.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc.hpp"


// this opencv: https://stackoverflow.com/a/34720686/2482283
// cubemap: https://www.khronos.org/opengl/wiki/Cubemap_Texture
// python correct: https://github.com/sunset1995/py360convert
// imagemagick not correct: http://www.fmwconcepts.com/imagemagick/sphericalpano2cube/index.php
// others: http://paulbourke.net/panorama/cubemaps/#2
// slow python: https://stackoverflow.com/a/29681646/2482283

// Define our six cube faces.
// 0 - 3 are side faces, clockwise order
// 4 and 5 are top and bottom, respectively
//float faceTransform[6][2] =
//{
//    {0, 0},
//    {M_PI / 2, 0},
//    {M_PI, 0},
//    {-M_PI / 2, 0},
//    {0, -M_PI / 2},
//    {0, M_PI / 2}
//};


/// ktx and gl order: +X, -X, +Y, -Y, +Z, -Z ~ GL_TEXTURE_CUBE_MAP_POSITIVE_X+i
/// +x right +y top +z front
static float faceTransform[6][2] =
{
    // left front right back top bottom
    {M_PI / 2, 0},      // right    +x
    {-M_PI / 2, 0},     // left     -x
    {0, -M_PI / 2},     // top      +y
    {0, M_PI / 2},      // bottom   -y
    {0, 0},             // front    +z
    {M_PI, 0},          // back     -z
};

// Map a part of the equirectangular panorama (in) to a cube face
// (face). The ID of the face is given by faceId. The desired
// width and height are given by width and height.
inline void createCubeMapFace(const cv::Mat &in, cv::Mat &face,
        int faceId = 0, int width = -1,
        int height = -1) {

    float inWidth = in.cols;
    float inHeight = in.rows;
    
    if (width < 0) {
        width = inWidth / 4;
        height = width;
    }
    

    // Allocate map
    cv::Mat mapx(height, width, CV_32F);
    cv::Mat mapy(height, width, CV_32F);

    // Calculate adjacent (ak) and opposite (an) of the
    // triangle that is spanned from the sphere center
    //to our cube face.
    const float an = sin(M_PI / 4);
    const float ak = cos(M_PI / 4);

    const float ftu = faceTransform[faceId][0];
    const float ftv = faceTransform[faceId][1];

    // For each point in the target image,
    // calculate the corresponding source coordinates.
    for(int y = 0; y < height; y++) {
        for(int x = 0; x < width; x++) {

            // Map face pixel coordinates to [-1, 1] on plane
            float nx = (float)y / (float)height - 0.5f;
            float ny = (float)x / (float)width - 0.5f;

            nx *= 2;
            ny *= 2;

            // Map [-1, 1] plane coords to [-an, an]
            // thats the coordinates in respect to a unit sphere
            // that contains our box.
            nx *= an;
            ny *= an;

            float u, v;

            // Project from plane to sphere surface.
            if(ftv == 0) {
                // Center faces
                u = atan2(nx, ak);
                v = atan2(ny * cos(u), ak);
                u += ftu;
            } else if(ftv > 0) {
                // Bottom face
                float d = sqrt(nx * nx + ny * ny);
                v = M_PI / 2 - atan2(d, ak);
                u = atan2(ny, nx);
            } else {
                // Top face
                float d = sqrt(nx * nx + ny * ny);
                v = -M_PI / 2 + atan2(d, ak);
                u = atan2(-ny, nx);
            }

            // Map from angular coordinates to [-1, 1], respectively.
            u = u / (M_PI);
            v = v / (M_PI / 2);

            // Warp around, if our coordinates are out of bounds.
            while (v < -1) {
                v += 2;
                u += 1;
            }
            while (v > 1) {
                v -= 2;
                u += 1;
            }

            while(u < -1) {
                u += 2;
            }
            while(u > 1) {
                u -= 2;
            }

            // Map from [-1, 1] to in texture space
            u = u / 2.0f + 0.5f;
            v = v / 2.0f + 0.5f;

            u = u * (inWidth - 1);
            v = v * (inHeight - 1);

            // Save the result for this pixel in map
            mapx.at<float>(x, y) = u;
            mapy.at<float>(x, y) = v;
        }
    }

    // Recreate output image if it has wrong size or type.
    if(face.cols != width || face.rows != height ||
        face.type() != in.type()) {
        face = cv::Mat(width, height, in.type());
    }

    // Do actual resampling using OpenCV's remap
    remap(in, face, mapx, mapy, cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
    if (faceId == 2) {
        // top, rotation clockwise
        cv::rotate(face, face, cv::ROTATE_90_CLOCKWISE );
    } else if (faceId == 3) {
        // bottom, rotation anti-clockwise
        cv::rotate(face, face, cv::ROTATE_90_COUNTERCLOCKWISE );
    }
}

bool pano2cube(std::string inpath, std::vector<std::string>& outpaths, int width, std::string mergedPath) {
    
    // check input file and out file count
    assert(outpaths.size() == 6);
    if (outpaths.size() != 6) {
        return false;
    }
    
    // read input
    cv::Mat in;
    in = cv::imread( inpath, cv::IMREAD_UNCHANGED ); // 50ms
    if (in.rows == 0) {
        printf("file not exsit: %s\n", inpath.c_str());
        return false;
    }
    
    // convert to cubemap
    // +x -x +y -x +z -z
    cv::Mat outs[6];
    for (int faceid = 0; faceid < 6; faceid++) {
        cv::Mat& out = outs[faceid];
        createCubeMapFace(in, out, faceid, width, width);  // 10ms
        cv::imwrite(outpaths[faceid], out);                // 15ms
    }
    
    if (mergedPath != "") {
        // merge
        // right    +x
        // left     -x
        // top      +y
        // bottom   -y
        // front    +z
        // back     -z
        
        int w = outs[0].cols;
        int h = outs[0].rows;
        cv::Mat merged(h*3, w*4, in.type());
        
        //cv::Mat imgPanelRoi(merged, cv::Rect(0, 0, w, h));
        outs[1].copyTo(merged(cv::Rect(0, h, w, h))); // left
        outs[4].copyTo(merged(cv::Rect(w, h, w, h))); // front
        outs[0].copyTo(merged(cv::Rect(2*w, h, w, h))); // right
        outs[5].copyTo(merged(cv::Rect(3*w, h, w, h))); // back
        outs[2].copyTo(merged(cv::Rect(1*w, 0, w, h))); // top
        outs[3].copyTo(merged(cv::Rect(1*w, 2*h, w, h))); // bottom
        
        cv::imwrite(mergedPath, merged);
    }
    
    return true;
}


#if 1

static inline std::string getCurrentTimestamp() {
    using std::chrono::system_clock;
    auto currentTime = std::chrono::system_clock::now();
    char buffer[80];
    
    auto transformed = currentTime.time_since_epoch().count() / 1000;
    
    auto millis = transformed % 1000;
    
    std::time_t tt;
    tt = system_clock::to_time_t ( currentTime );
    auto timeinfo = localtime (&tt);
    strftime (buffer,80,"%F %H:%M:%S",timeinfo);
    sprintf(buffer, "%s:%03d",buffer,(int)millis);
    
    return std::string(buffer);
}


static void testconvert() {
    std::string file_path = __FILE__;
#ifdef _WIN64
    std::string dir_path = file_path.substr(0, file_path.rfind("/"));
    std::string filename = dir_path + "/auto_stop_spherical_pano.jpg";
    
    std::vector<std::string> outfiles = {
        dir_path + "/auto_stop_spherical_pano0.png",
        dir_path + "/auto_stop_spherical_pano1.png",
        dir_path + "/auto_stop_spherical_pano2.png",
        dir_path + "/auto_stop_spherical_pano3.png",
        dir_path + "/auto_stop_spherical_pano4.png",
        dir_path + "/auto_stop_spherical_pano5.png",
    };
    std::string mergedPath = dir_path + "/auto_stop_spherical_pano6.png";
#else
    std::string dir_path = file_path.substr(0, file_path.rfind("\\"));
    std::string filename = dir_path + "\\auto_stop_spherical_pano.jpg";
    
    std::vector<std::string> outfiles = {
        dir_path + "\\auto_stop_spherical_pano0.png",
        dir_path + "\\auto_stop_spherical_pano1.png",
        dir_path + "\\auto_stop_spherical_pano2.png",
        dir_path + "\\auto_stop_spherical_pano3.png",
        dir_path + "\\auto_stop_spherical_pano4.png",
        dir_path + "\\auto_stop_spherical_pano5.png",
    };
    std::string mergedPath = dir_path + "\\auto_stop_spherical_pano6.png";
#endif
    
    std::cout << "begin: " << getCurrentTimestamp() << std::endl;
    pano2cube(filename, outfiles, -1, mergedPath);
    std::cout << "end: " << getCurrentTimestamp() << std::endl;
}


static void testconvertAndView() {
    std::string file_path = __FILE__;
    
#ifdef _WIN64
    std::string dir_path = file_path.substr(0, file_path.rfind("\\"));
    std::string filename = dir_path + "\\auto_stop_spherical_pano.jpg";
#else
    std::string dir_path = file_path.substr(0, file_path.rfind("/"));
    std::string filename = dir_path + "/auto_stop_spherical_pano.jpg";
#endif
    
    cv::Mat in;
    in = cv::imread( filename, cv::IMREAD_UNCHANGED );
    
    //std::time_t t = std::time(0);
    //std::cout << "begin: " << std::ctime(&t) << std::endl;
    std::cout << "begin: " << getCurrentTimestamp() << std::endl;
    for (int face = 0; face < 6; face++) {
        
        cv::Mat out;
        
        createCubeMapFace(in, out, face, 512, 512);
        
        // create window
         cv::namedWindow("Display Image", cv::WINDOW_AUTOSIZE );
        
        // show window
        cv::imshow("Display Image", out);
        cv::waitKey(0);
    }
    
    std::cout << "end: " << getCurrentTimestamp() << std::endl;
}


int testmain(int argc, const char * argv[]) {

    testconvert();
    testconvertAndView();

    return 0;
}

int main(int argc, const char * argv[]) {
    
    if (argc < 2) {
        printf("no input file, correct format:\npano2cube input_file\n");
        return -1;
    }

    std::string file_path = argv[1];
    
    std::string dir_path = ".";
    std::string file_name = file_path;
    if (file_path.rfind("/") != std::string::npos) {
        dir_path = file_path.substr(0, file_path.rfind("/"));
        file_name = file_path.substr(file_path.rfind("/") + 1);
    }
    file_name = file_name.substr(0, file_path.rfind("."));
    
    std::string infile = file_path;

    char const *slash = "/";
#if defined(_WINDOWS)
	slash = "\\";
#endif

    std::vector<std::string> outfiles = {
        dir_path + slash + file_name + "0.png",
        dir_path + slash + file_name + "1.png",
        dir_path + slash + file_name + "2.png",
        dir_path + slash + file_name + "3.png",
        dir_path + slash + file_name + "4.png",
        dir_path + slash + file_name + "5.png",
    };
	std::string mergedPath = dir_path + slash + file_name + "6.png";
    
    pano2cube(infile, outfiles, -1, mergedPath);
    
    printf("save images to path:\n");
    for (auto& path : outfiles) {
        printf("    %s\n", path.c_str());
    }
    
    return 0;
}

#endif
