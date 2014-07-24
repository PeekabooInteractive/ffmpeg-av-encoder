package com.peekaboo.videoencoder;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.app.Notification;
import android.app.Notification.Builder;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.util.Log;

import com.google.android.gms.auth.GoogleAuthException;
import com.google.android.gms.auth.GoogleAuthUtil;
import com.google.android.gms.auth.GooglePlayServicesAvailabilityException;
import com.google.android.gms.auth.UserRecoverableAuthException;
import com.google.android.gms.common.Scopes;
import com.google.api.client.googleapis.extensions.android.gms.auth.GoogleAccountCredential;
import com.google.api.client.googleapis.json.GoogleJsonResponseException;
import com.google.api.client.googleapis.media.MediaHttpUploader;
import com.google.api.client.googleapis.media.MediaHttpUploaderProgressListener;
import com.google.api.client.http.InputStreamContent;
import com.google.api.client.util.ExponentialBackOff;
import com.google.api.services.youtube.YouTube;
import com.google.api.services.youtube.YouTubeScopes;
import com.google.api.services.youtube.model.Video;
import com.google.api.services.youtube.model.VideoSnippet;
import com.google.api.services.youtube.model.VideoStatus;
import com.unity3d.player.UnityPlayerActivity;
//import com.google.api.services.samples.youtube.cmdline.Auth;



public class VideoUploader extends UnityPlayerActivity {
	
	 /**
     * Define a global instance of a Youtube object, which will be used
     * to make YouTube Data API requests.
     */
	private static YouTube youtube;

    /**
     * Define a global variable that specifies the MIME type of the video
     * being uploaded.
     */
    private static final String VIDEO_FILE_FORMAT = "video/*";

    private static final String SAMPLE_VIDEO_FILENAME = "sample-video.mp4";
    
    public static Context context;
    
    public static Bundle bundle;
    
    public static VideoUploader activity;
    
    private static GoogleAccountCredential credential;
    
    public static final String KEY = "231250131118-4r3gg0j71q2bp71jsc4605sh1m290knn.apps.googleusercontent.com";

    public static final String[] SCOPES = {Scopes.PLUS_ME,YouTubeScopes.YOUTUBE_UPLOAD,YouTubeScopes.YOUTUBE_READONLY,YouTubeScopes.YOUTUBE};
    
    private static SharedPreferences settings;

    protected void onCreate(Bundle savedInstanceState) {

		// call UnityPlayerActivity.onCreate()
		super.onCreate(savedInstanceState);
		settings =  getPreferences(Context.MODE_PRIVATE);
		context = this;
		activity = this;
		bundle = savedInstanceState;
		// print debug message to logcat
		Log.e("VideoUploader", "onCreate called!");
    }

    public void onBackPressed(){
    	//UploadVideo("");
	    // instead of calling UnityPlayerActivity.onBackPressed() we just ignore the back button event
	    // super.onBackPressed();
    }
  
    public static void  UploadVideo(String path){
    	Log.e("VideoUploader", "EXECUTE!!!!!!!!!!");
    	
    	
        // This OAuth 2.0 access scope allows an application to upload files
        // to the authenticated user's YouTube channel, but doesn't allow
        // other types of access.
        //List<String> scopes = Lists.newArrayList("https://www.googleapis.com/auth/youtube.upload");
    	Account mAccount;
        try {
        	mAccount = AccountManager.get(context).getAccountsByType(GoogleAuthUtil.GOOGLE_ACCOUNT_TYPE)[0];
        	
        	/*String tokens = GoogleAuthUtil.getToken(activity,mAccount.name, "oauth2:client_id:"+ KEY +":api_scope:"+Scopes.PLUS_ME+" "+Scopes.GAMES+" "+Scopes.PROFILE+" "+Scopes.PLUS_LOGIN+" "+Scopes.APP_STATE+" https://www.googleapis.com/auth/plus.login "+
        			Scopes.CLOUD_SAVE+" "+Scopes.DRIVE_APPFOLDER+" "+YouTubeScopes.YOUTUBEPARTNER+" "+YouTubeScopes.YOUTUBEPARTNER_CHANNEL_AUDIT+" "
        			+YouTubeScopes.YOUTUBE_READONLY+" "+YouTubeScopes.YOUTUBE_UPLOAD+" "+YouTubeScopes.YOUTUBE+" "+YouTubeScopes.YOUTUBEPARTNER,bundle);*/
        	
        	/*String tokens = GoogleAuthUtil.getTokenWithNotification(activity,mAccount.name, "oauth2:"+Scopes.PLUS_ME+" "+Scopes.GAMES+" "+Scopes.PROFILE+" "+Scopes.PLUS_LOGIN
        			+" "+YouTubeScopes.YOUTUBE_READONLY+" "+YouTubeScopes.YOUTUBE_UPLOAD+" "+YouTubeScopes.YOUTUBE,null);*/
        	
        	Log.e("VideoUploader", "Ready credentials");     
        	
        	//credential = GoogleAccountCredential.usingAudience(activity, "audience:server:client_id:"+KEY+":api_scope:"+Scopes.PLUS_ME+" "+YouTubeScopes.YOUTUBE_READONLY+" "+YouTubeScopes.YOUTUBE_UPLOAD);
        
        	credential = GoogleAccountCredential. usingOAuth2(context,Arrays.asList(SCOPES));// Collections.singleton(YouTubeScopes.YOUTUBE_UPLOAD));
        	
        	//SharedPreferences settings = context.getSharedPreferences("", Context.MODE_PRIVATE);
        	 //SharedPreferences settings = context.getSharedPreferences(Context.MODE_PRIVATE,0);
        	
        	
        	//SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(context);
    		//String mChosenAccountName = sp.getString("accountName", null);
        	
        	credential.setSelectedAccountName(mAccount.name);
        	
        	credential.getToken();
        	
        	Log.e("VideoUploader", "settings "+settings.getString("accountName", null));
        	Log.e("VideoUploader"," Account "+mAccount.name);
    
    	    credential.setBackOff(new ExponentialBackOff());
        	
        	
        	Log.e("VideoUploader", "youtube credentials");
    		youtube = new YouTube.Builder(Auth.HTTP_TRANSPORT_DEFAULT, Auth.JSON_FACTORY_DEFAULT, credential).setApplicationName("Video-trial").build();
        	        	
        	//InputStream input = context.getAssets().open("client_secrets.json");
        	// Authorize the request.
            //Credential credential = Auth.authorize(scopes, "uploadvideo",input,path);

            
            // This object is used to make YouTube Data API requests.
            //youtube = new YouTube.Builder(Auth.HTTP_TRANSPORT, Auth.JSON_FACTORY, credential).setApplicationName("VideoUploader").build();

            Log.e("VideoUploader","Uploading: " + SAMPLE_VIDEO_FILENAME);

            // Add extra information to the video before uploading.
            Video videoObjectDefiningMetadata = new Video();

            // Set the video to be publicly visible. This is the default
            // setting. Other supporting settings are "unlisted" and "private."
            VideoStatus status = new VideoStatus();
            status.setPrivacyStatus("public");
            videoObjectDefiningMetadata.setStatus(status);

            // Most of the video's metadata is set on the VideoSnippet object.
            VideoSnippet snippet = new VideoSnippet();

            // This code uses a Calendar instance to create a unique name and
            // description for test purposes so that you can easily upload
            // multiple files. You should remove this code from your project
            // and use your own standard names instead.
           // Calendar cal = Calendar.getInstance();
            snippet.setTitle("Test Upload via Java on ");
            snippet.setDescription("Video uploaded via YouTube Data API V3 using the Java library");

            // Set the keyword tags that you want to associate with the video.
            List<String> tags = new ArrayList<String>();
            tags.add("test");
            tags.add("example");
            tags.add("java");
            tags.add("YouTube Data API V3");
            tags.add("erase me");
            snippet.setTags(tags);

            // Add the completed snippet object to the video resource.
            videoObjectDefiningMetadata.setSnippet(snippet);

            InputStreamContent mediaContent = new InputStreamContent(VIDEO_FILE_FORMAT,context.getAssets().open(SAMPLE_VIDEO_FILENAME));

            // Insert the video. The command sends three arguments. The first
            // specifies which information the API request is setting and which
            // information the API response should return. The second argument
            // is the video resource that contains metadata about the new video.
            // The third argument is the actual video content.
            YouTube.Videos.Insert videoInsert = youtube.videos().insert("snippet,statistics,status", videoObjectDefiningMetadata, mediaContent);

            // Set the upload type and add an event listener.
            MediaHttpUploader uploader = videoInsert.getMediaHttpUploader();

            // Indicate whether direct media upload is enabled. A value of
            // "True" indicates that direct media upload is enabled and that
            // the entire media content will be uploaded in a single request.
            // A value of "False," which is the default, indicates that the
            // request will use the resumable media upload protocol, which
            // supports the ability to resume an upload operation after a
            // network interruption or other transmission failure, saving
            // time and bandwidth in the event of network failures.
            uploader.setDirectUploadEnabled(false);

            MediaHttpUploaderProgressListener progressListener = new MediaHttpUploaderProgressListener() {
                public void progressChanged(MediaHttpUploader uploader) throws IOException {
                    switch (uploader.getUploadState()) {
                        case INITIATION_STARTED:
                            System.out.println("Initiation Started");
                            break;
                        case INITIATION_COMPLETE:
                            System.out.println("Initiation Completed");
                            break;
                        case MEDIA_IN_PROGRESS:
                            System.out.println("Upload in progress");
                            System.out.println("Upload percentage: " + uploader.getProgress());
                            break;
                        case MEDIA_COMPLETE:
                            System.out.println("Upload Completed!");
                            break;
                        case NOT_STARTED:
                            System.out.println("Upload Not Started!");
                            break;
                    }
                }
            };
            uploader.setProgressListener(progressListener);

            // Call the API and upload the video.
            Video returnedVideo = videoInsert.execute();

            // Print data about the newly inserted video from the API response.
            Log.e("VideoUploader","\n================== Returned Video ==================\n");
            Log.e("VideoUploader","  - Id: " + returnedVideo.getId());
            Log.e("VideoUploader","  - Title: " + returnedVideo.getSnippet().getTitle());
            Log.e("VideoUploader","  - Tags: " + returnedVideo.getSnippet().getTags());
            Log.e("VideoUploader","  - Privacy Status: " + returnedVideo.getStatus().getPrivacyStatus());
            Log.e("VideoUploader","  - Video Count: " + returnedVideo.getStatistics().getViewCount());

        } catch (GoogleJsonResponseException e) {
            System.err.println("GoogleJsonResponseException code: " + e.getDetails().getCode() + " : " + e.getDetails().getMessage());
            e.printStackTrace();
        } catch (IOException e) {
            System.err.println("IOException: " + e.getMessage());
            e.printStackTrace();
        }catch (GooglePlayServicesAvailabilityException playEx) {
        	System.err.println("GooglePlayServicesAvailabilityException authentication exception: " + playEx.getMessage());
        }catch (UserRecoverableAuthException recoverableException) {
        	activity.startActivityForResult(recoverableException.getIntent(), 0);        	
        	System.err.println("UserRecoverableAuthException authentication exception: " + recoverableException.getMessage());
        } catch (GoogleAuthException authEx) {
            // This is likely unrecoverable.
        	 System.err.println("Unrecoverable authentication exception: " + authEx.getMessage());
        } catch (Throwable t) {
            System.err.println("Throwable: " + t.getMessage());
            t.printStackTrace();
        }	
    	
    }
  
 
}