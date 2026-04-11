//
//  bundle.h
//  MotionBlur
//
//  Created by administrator on 4/10/25.
//

#include <string>
std::string getResourcePath();
std::string getModelResourcePath();
std::string fetchWithAuthSync(std::string urlStr, std::string user, std::string pass);
