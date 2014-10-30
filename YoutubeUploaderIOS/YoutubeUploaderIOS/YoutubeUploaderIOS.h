
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

@class GTMOAuth2Authentication;

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


