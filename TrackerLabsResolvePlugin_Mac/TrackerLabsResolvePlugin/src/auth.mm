//
//  bundle.mm
//  MotionBlur
//
//  Created by administrator on 4/10/25.
//
#include "auth.h"
#import <CoreFoundation/CoreFoundation.h>
#import <Foundation/Foundation.h>
std::string fetchWithAuthSync(std::string urlStr, std::string user, std::string pass) {
    __block std::string result = "";
    
    // 1. Prepare URL and Basic Auth String
    NSURL *url = [NSURL URLWithString:[NSString stringWithUTF8String:urlStr.c_str()]];
    NSString *authStr = [NSString stringWithFormat:@"%s:%s", user.c_str(), pass.c_str()];
    NSData *authData = [authStr dataUsingEncoding:NSUTF8StringEncoding];
    NSString *base64Auth = [authData base64EncodedStringWithOptions:0];
    
    // 2. Create Request with Authorization Header
    NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:url];
    [request setHTTPMethod:@"GET"];
    [request setValue:[NSString stringWithFormat:@"Basic %@", base64Auth] forHTTPHeaderField:@"Authorization"];
    
    // 3. Setup Semaphore for Synchronous Wait
    dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);
    
    [[[NSURLSession sharedSession] dataTaskWithRequest:request
        completionHandler:^(NSData *data, NSURLResponse *response, NSError *error) {
        
        if (data && !error) {
            result = std::string((const char *)[data bytes], [data length]);
        } else if (error) {
            result = "Error: " + std::string([error.localizedDescription UTF8String]);
        }
        
        // 4. Wake up the original thread
        dispatch_semaphore_signal(semaphore);
    }] resume];
    
    // 5. Block the thread until response (or timeout)
    dispatch_semaphore_wait(semaphore, DISPATCH_TIME_FOREVER);
    
    return result;
}
std::string getResourcePath()
{
    std::string str="";

    CFBundleRef bundle = CFBundleGetBundleWithIdentifier(CFSTR("com.adobe.AfterEffects.MotionBlur"));

    if(bundle==NULL)
    {
        return str;
    }
    
    CFURLRef fileURL = CFBundleCopyResourceURL(bundle, CFSTR("kernel"), CFSTR("pk"), NULL);

    
    if (fileURL)
    {
        char path[PATH_MAX];
        if (CFURLGetFileSystemRepresentation(fileURL, true, (UInt8*)path, sizeof(path))) {
            str = path;
        }
        CFRelease(fileURL);
    }
    CFRelease(bundle);
    return str;
}
std::string getModelResourcePath()
{
    std::string str="";

    CFBundleRef bundle = CFBundleGetBundleWithIdentifier(CFSTR("com.adobe.AfterEffects.MotionBlur"));

    if(bundle==NULL)
    {
        return str;
    }
    
    CFURLRef fileURL = CFBundleCopyResourceURL(bundle, NULL,NULL,CFSTR("model"));

    
    if (fileURL)
    {
        char path[PATH_MAX];
        if (CFURLGetFileSystemRepresentation(fileURL, true, (UInt8*)path, sizeof(path))) {
            str = path;
        }
        CFRelease(fileURL);
    }
    CFRelease(bundle);
    return str;
}
