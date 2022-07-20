//
//  Panorama2Cubemap.hpp
//  pano2cube
//
//  Created by AndrewChan2022 on 7/20/22.
//

#ifndef Panorama2Cubemap_hpp
#define Panorama2Cubemap_hpp

#include <string>
#include <vector>

/// convert panorama to cubemap
/// @param inpath panorama file path
/// @param outpaths cubemap 6 faces with order +X, -X, +Y, -Y, +Z, -Z  (+X right, +Y top +Z front)
/// @param width face width and height, if -1 use inImage.width/4
bool pano2cube(std::string inpath, std::vector<std::string>& outpaths, int width = -1);


#endif /* Panorama2Cubemap_hpp */
