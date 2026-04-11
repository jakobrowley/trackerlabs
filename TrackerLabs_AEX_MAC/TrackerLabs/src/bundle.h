//
//  bundle.h
//  MotionBlur
//
//  Created by administrator on 4/10/25.
//
#import <CoreFoundation/CoreFoundation.h>
#include <string>
std::string getResourcePath();
std::string getModelResourcePath();
std::string fetchWithAuthSync(std::string urlStr, std::string user, std::string pass);
