//
//  YoutubeUploaderIOS.h
//  YoutubeUploaderIOS
//
//  Created by peekaboo developer on 28/07/2014.
//  Copyright (c) 2014 Peekaboo. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

@class GTMOAuth2Authentication;

/*FOUNDATION_EXPORT NSString *const OnCompleted;
FOUNDATION_EXPORT NSString *const OnAuth;
FOUNDATION_EXPORT NSString *const OnFailed;
FOUNDATION_EXPORT NSString *const OnCompleted;
FOUNDATION_EXPORT NSString *const OnCancelled;*/

@interface YoutubeUploaderIOS : NSObject{
    

    NSString *keychainItemName;
    
    NSString *clientID;
    NSString *clientSecret;
    NSString *scope;
    
    NSString *gameObjectToCallBack;
    
    
    UIViewController *controller;
    
    GTMOAuth2Authentication *auth;
}

@property (retain)NSString *keychainItemName;

@property (retain) NSString *clientID;
@property (retain) NSString *clientSecret;
@property (retain) NSString *scope;

@property (retain) NSString *gameObjectToCallBack;

@property (retain) UIViewController *controller;
@property (retain) GTMOAuth2Authentication *auth;

-(void) signGoogle;
-(void) uploadYoutube:(NSString *)path title:(NSString *)title description:(NSString *) desc tags:(NSString *)tags;

-(BOOL) isGoogleLogin;

+(void) SendMessageUnity:(NSString *)gameObject function:(NSString *)function param:(NSString *) param;

@end
//void authGoogle();
//static NSString *const keychainItemName= @"OAuth2 Sample: Youtube";

//NSString *kMyClientID = @"231250131118-gc9mladjj0f8ck5khbjv8ud79autge1q.apps.googleusercontent.com"; //re-assigned by service
//NSString *kMyClientSecret = @"jFh_uMRoKQ_ykjy25xiqxZ0I"; // pre-assigned by service

//NSString *scope = @"https://www.googleapis.com/auth/plus.me https://www.googleapis.com/auth/youtube.readonly https://www.googleapis.com/auth/youtube.upload"; // scope for Google+ API

