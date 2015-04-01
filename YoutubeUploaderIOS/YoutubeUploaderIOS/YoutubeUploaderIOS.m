
#import "YoutubeUploaderIOS.h"

#import "GTMOAuth2SignIn.h"
#import "GTMOAuth2ViewControllerTouch.h"

#import "GTMHTTPUploadFetcher.h"
#import "GTMHTTPFetcherLogging.h"

#import "GTLYouTube.h"


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
NSString *const OnCompletedVideoInfo = @"OnCompletedVideoInfo";

NSString *const OnAuth = @"OnAuth";
NSString *const OnFailed = @"OnFailed";
NSString *const OnCancelled = @"OnCancelled";

-(void) signGoogle{
    
    scope = @"https://www.googleapis.com/auth/plus.me https://www.googleapis.com/auth/youtube.readonly https://www.googleapis.com/auth/youtube.upload";
    
    SEL finishedSel = @selector(viewController:finishedWithAuth:error:);
    
    GTMOAuth2ViewControllerTouch *viewController;
    viewController = [GTMOAuth2ViewControllerTouch controllerWithScope:scope
                                                              clientID:clientID
                                                          clientSecret:clientSecret
                                                      keychainItemName:keychainItemName
                                                              delegate:self
                                                      finishedSelector:finishedSel];
    
    viewController.modalTransitionStyle = UIModalTransitionStyleCrossDissolve;

    NSDictionary *params = [NSDictionary dictionaryWithObject:@"en"
                                                       forKey:@"hl"];
    viewController.signIn.additionalAuthorizationParameters = params;

    NSString *html = @"<html><body bgcolor=silver><div align=center>Loading sign-in page...</div></body></html>";
    viewController.initialHTMLString = html;

    [self.controller presentViewController:viewController animated:YES completion:nil];
}


- (void)viewController:(GTMOAuth2ViewControllerTouch *)viewController
      finishedWithAuth:(GTMOAuth2Authentication *)aux_auth
                 error:(NSError *)error {
    if (error != nil) {
        [YoutubeUploaderIOS SendMessageUnity:gameObjectToCallBack function:OnFailed param:error.description];
        // Authentication failed
    } else {
        self.auth = aux_auth;
        [YoutubeUploaderIOS SendMessageUnity:gameObjectToCallBack function:OnAuth param:@""];
    }
    [self.controller dismissViewControllerAnimated:YES completion:nil];
}

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

    if ([tags length] > 0) {
        snippet.tags = [tags componentsSeparatedByString:@","];
    }
    
    GTLYouTubeVideo *video = [GTLYouTubeVideo object];
    video.status = status;
    video.snippet = snippet;
    
    
    NSFileHandle *fileHandle = [NSFileHandle fileHandleForReadingAtPath:path];
    
    
    if (fileHandle) {
        NSLog(@"File found");
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
                                                        [YoutubeUploaderIOS SendMessageUnity:gameObjectToCallBack function:OnCompletedVideoInfo param:uploadedVideo.identifier];
                                                    } else {
                                                        NSLog(@"ERROR");
                                                        [YoutubeUploaderIOS SendMessageUnity:gameObjectToCallBack function:OnFailed param:error.description];
                                                    }
                                                }];
        
        GTMHTTPUploadFetcher *uploadFetcher = (GTMHTTPUploadFetcher *)[uploadFileTicket objectFetcher];
        uploadFetcher.locationChangeBlock = ^(NSURL *url) {
            //uploadLocationURL = url;
            
        };

        
    }
    else{
        NSLog(@"File not found");
    
        [YoutubeUploaderIOS SendMessageUnity:gameObjectToCallBack function:OnFailed param:@"File not found"];
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

+(void) SendMessageUnity:(NSString *)gameObject function:(NSString *)function param:(NSString *) param{
     UnitySendMessage([gameObject cStringUsingEncoding:NSUTF8StringEncoding], [function cStringUsingEncoding:NSUTF8StringEncoding], [param cStringUsingEncoding:NSUTF8StringEncoding]);
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
        [YoutubeUploaderIOS SendMessageUnity:uploader.gameObjectToCallBack function:OnAuth param:@""];
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






