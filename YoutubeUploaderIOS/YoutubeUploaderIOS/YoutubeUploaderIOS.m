//
//  YoutubeUploaderIOS.m
//  YoutubeUploaderIOS
//
//  Created by peekaboo developer on 28/07/2014.
//  Copyright (c) 2014 Peekaboo. All rights reserved.
//

#import "YoutubeUploaderIOS.h"

#import "GTMOAuth2SignIn.h"
#import "GTMOAuth2ViewControllerTouch.h"

#import "GTMHTTPUploadFetcher.h"
#import "GTMHTTPFetcherLogging.h"

#import "GTLYouTube.h"



/*@interface YoutubeUploaderIOS()
-(void) signGoogle;
- (void)viewController:(GTMOAuth2ViewControllerTouch *)viewController
      finishedWithAuth:(GTMOAuth2Authentication *)auth
                 error:(NSError *)error;
- (void)incrementNetworkActivity:(NSNotification *)notify;
- (void)decrementNetworkActivity:(NSNotification *)notify;
- (void)signInNetworkLostOrFound:(NSNotification *)notify;
- (GTMOAuth2Authentication *)authForDailyMotion;
- (void)doAnAuthenticatedAPIFetch;
- (void)displayAlertWithMessage:(NSString *)str;
- (BOOL)shouldSaveInKeychain;
- (void)saveClientIDValues;
- (void)loadClientIDValues;

@end*/

@class GTMOAuth2Authentication;

@implementation YoutubeUploaderIOS

@synthesize controller;
@synthesize auth;

@synthesize clientID;
@synthesize clientSecret;
@synthesize keychainItemName;
@synthesize scope;

@synthesize gameObjectToCallBack;

NSString *const OnCompleted = @"OnCompleted";

NSString *const OnAuth = @"OnAuth";
NSString *const OnFailed = @"OnFailed";
NSString *const OnCancelled = @"OnCancelled";

-(void) signGoogle{
    // Note:
    // GTMOAuth2ViewControllerTouch is not designed to be reused. Make a new
    // one each time you are going to show it.
    scope = @"https://www.googleapis.com/auth/plus.me https://www.googleapis.com/auth/youtube.readonly https://www.googleapis.com/auth/youtube.upload";
    // Display the autentication view.
    SEL finishedSel = @selector(viewController:finishedWithAuth:error:);
    
    //clientID = @"231250131118-gc9mladjj0f8ck5khbjv8ud79autge1q.apps.googleusercontent.com";
    //clientSecret = @"jFh_uMRoKQ_ykjy25xiqxZ0I";
    
    
    GTMOAuth2ViewControllerTouch *viewController;
    viewController = [GTMOAuth2ViewControllerTouch controllerWithScope:scope
                                                              clientID:clientID
                                                          clientSecret:clientSecret
                                                      keychainItemName:keychainItemName
                                                              delegate:self
                                                      finishedSelector:finishedSel];
    
    viewController.modalTransitionStyle = UIModalTransitionStyleCrossDissolve;
    // You can set the title of the navigationItem of the controller here, if you
    // want.
    
    // If the keychainItemName is not nil, the user's authorization information
    // will be saved to the keychain. By default, it saves with accessibility
    // kSecAttrAccessibleAfterFirstUnlockThisDeviceOnly, but that may be
    // customized here. For example,
    //
    //   viewController.keychainItemAccessibility = kSecAttrAccessibleAlways;
    
    // During display of the sign-in window, loss and regain of network
    // connectivity will be reported with the notifications
    // kGTMOAuth2NetworkLost/kGTMOAuth2NetworkFound
    //
    // See the method signInNetworkLostOrFound: for an example of handling
    // the notification.
    
    // Optional: Google servers allow specification of the sign-in display
    // language as an additional "hl" parameter to the authorization URL,
    // using BCP 47 language codes.
    //
    // For this sample, we'll force English as the display language.
    NSDictionary *params = [NSDictionary dictionaryWithObject:@"en"
                                                       forKey:@"hl"];
    viewController.signIn.additionalAuthorizationParameters = params;
    
    // By default, the controller will fetch the user's email, but not the rest of
    // the user's profile.  The full profile can be requested from Google's server
    // by setting this property before sign-in:
    //
    //   viewController.signIn.shouldFetchGoogleUserProfile = YES;
    //
    // The profile will be available after sign-in as
    //
    //   NSDictionary *profile = viewController.signIn.userProfile;
    
    // Optional: display some html briefly before the sign-in page loads
    NSString *html = @"<html><body bgcolor=silver><div align=center>Loading sign-in page...</div></body></html>";
    viewController.initialHTMLString = html;
    
    //[self.controller.navigationController pushViewController:viewController animated:YES];
    
    //[self.controller.navigationController pushViewController:viewController animated:YES];
    [self.controller presentViewController:viewController animated:YES completion:nil];
    
    
    // The view controller will be popped before signing in has completed, as
    // there are some additional fetches done by the sign-in controller.
    // The kGTMOAuth2UserSignedIn notification will be posted to indicate
    // that the view has been popped and those additional fetches have begun.
    // It may be useful to display a temporary UI when kGTMOAuth2UserSignedIn is
    // posted, just until the finished selector is invoked.
}


- (void)viewController:(GTMOAuth2ViewControllerTouch *)viewController
      finishedWithAuth:(GTMOAuth2Authentication *)aux_auth
                 error:(NSError *)error {
    if (error != nil) {
        UnitySendMessage(gameObjectToCallBack, OnFailed, "Authentication failed");
        // Authentication failed
    } else {
        self.auth = aux_auth;
        //[self.controller dismissModalViewControllerAnimated:YES];
        [self.controller dismissViewControllerAnimated:YES completion:nil];
        UnitySendMessage(gameObjectToCallBack, OnAuth, "");
    }
}
/*
- (void)authentication:(GTMOAuth2Authentication *)auth
               request:(NSMutableURLRequest *)request
     finishedWithError:(NSError *)error {
    if (error != nil) {
        // Authorization failed
    } else {
        // Authorization succeeded
    }
}


- (void)awakeFromNib {
    // Get the saved authentication, if any, from the keychain.
    GTMOAuth2Authentication *auth;
    auth = [GTMOAuth2ViewControllerTouch authForGoogleFromKeychainForName:keychainItemName
                                                                 clientID:kMyClientID
                                                             clientSecret:kMyClientSecret];
    
    // Retain the authentication object, which holds the auth tokens
    //
    // We can determine later if the auth object contains an access token
    // by calling its -canAuthorize method
    [self setAuthentication:auth];
}
*/

-(void) uploadYoutube:(NSString *)path title:(NSString *)title description:(NSString *)desc tags:(NSString *)tags{
    // Statu
    GTLYouTubeVideoStatus *status = [GTLYouTubeVideoStatus object];
    status.privacyStatus = @"public";
    
    // Snippet.
    GTLYouTubeVideoSnippet *snippet = [GTLYouTubeVideoSnippet object];
    snippet.title = title;
    if ([desc length] > 0) {
        snippet.descriptionProperty = desc;
    }
    snippet.tags = [tags componentsSeparatedByString:@","];
    /*if ([tags length] > 0) {
        snippet.tags = [tags componentsSeparatedByString:@","];
    }*/
    /*if ([_uploadCategoryPopup isEnabled]) {
        NSMenuItem *selectedCategory = [_uploadCategoryPopup selectedItem];
        snippet.categoryId = [selectedCategory representedObject];
    }*/
    
    GTLYouTubeVideo *video = [GTLYouTubeVideo object];
    video.status = status;
    video.snippet = snippet;
    
    
    NSFileHandle *fileHandle = [NSFileHandle fileHandleForReadingAtPath:path];
    
    //NSString *filename = [path lastPathComponent];
    
    if (fileHandle) {
        NSLog(@"File found");
        //NSString *mimeType = [self MIMETypeForFilename:filename defaultMIMEType:@"video/*"];
        GTLUploadParameters *uploadParameters = [GTLUploadParameters uploadParametersWithFileHandle:fileHandle
                                                                                           MIMEType:@"video/*"];
        
        GTLQueryYouTube *query = [GTLQueryYouTube queryForVideosInsertWithObject:video
                                                                            part:@"snippet,status"
                                                                            uploadParameters:uploadParameters];
        
        GTLServiceYouTube *service = [[GTLServiceYouTube alloc] init];
        service.authorizer = self.auth;
        
        GTLServiceTicket *uploadFileTicket = [service executeQuery:query
                                                completionHandler:^(GTLServiceTicket *ticket,
                                                                    GTLYouTubeVideo *uploadedVideo,
                                                                    NSError *error) {
                                                    // Callback
                                                    if (error == nil) {
                                                        NSLog(@"NO ERROR");
                                                        UnitySendMessage(gameObjectToCallBack, OnCompleted, "");
                                                    } else {
                                                        NSLog(@"ERROR");
                                                        UnitySendMessage(gameObjectToCallBack, OnFailed, error);
                                                    }
                                                }];
        
        GTMHTTPUploadFetcher *uploadFetcher = (GTMHTTPUploadFetcher *)[uploadFileTicket objectFetcher];
        uploadFetcher.locationChangeBlock = ^(NSURL *url) {
            //uploadLocationURL = url;
            
        };

        
    }
    else{
        NSLog(@"File not found");
    }
}

-(BOOL) isGoogleLogin{
    auth = nil;
    auth = [GTMOAuth2ViewControllerTouch authForGoogleFromKeychainForName:keychainItemName
                                                                 clientID:clientID
                                                             clientSecret:clientSecret];
    if(auth == nil){
        return  NO;
    }
    
    return auth.canAuthorize;
}

@end

extern UIViewController* UnityGetGLViewController();

YoutubeUploaderIOS *uploader;

void authGoogle(const char *clientID,const char *secret,const char *keyForSaveChain,const char *gameObjectToCallBack){
    
    uploader = [[YoutubeUploaderIOS alloc] init];
    
    uploader.controller = UnityGetGLViewController();
    
    uploader.clientID = [NSString stringWithUTF8String: clientID];
    uploader.clientSecret = [NSString stringWithUTF8String: secret];
    
    uploader.keychainItemName = [NSString stringWithUTF8String: keyForSaveChain];
    
    uploader.gameObjectToCallBack = [NSString stringWithUTF8String: gameObjectToCallBack];

    
    if(![uploader isGoogleLogin]){
        [uploader signGoogle];
    }
    else{
        NSLog(@"Saved");
    }

}

void uploadVideo(const char *pathVideo,const char *title,const char *description,const char *tags){
    NSString *auxPathVideo = [NSString stringWithUTF8String: pathVideo];
    NSString *auxTitle = [NSString stringWithUTF8String: title];
    NSString *auxDescription = [NSString stringWithUTF8String: description];
    NSString *auxTags = [NSString stringWithUTF8String: tags];
    
    
    [uploader uploadYoutube:auxPathVideo title:auxTitle description:auxDescription tags:auxTags];
}






