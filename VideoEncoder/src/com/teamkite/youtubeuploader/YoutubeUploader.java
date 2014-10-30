package com.teamkite.youtubeuploader;

import java.io.FileInputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import android.accounts.AccountManager;
import android.content.Intent;
import android.os.AsyncTask;
import android.os.Bundle;
import android.util.Log;

import com.google.android.gms.auth.GoogleAuthException;
import com.google.android.gms.auth.GoogleAuthUtil;
import com.google.android.gms.auth.GooglePlayServicesAvailabilityException;
import com.google.android.gms.auth.UserRecoverableAuthException;
import com.google.android.gms.common.AccountPicker;
import com.google.android.gms.common.Scopes;
import com.google.api.client.extensions.android.http.AndroidHttp;
import com.google.api.client.googleapis.extensions.android.gms.auth.GoogleAccountCredential;
import com.google.api.client.googleapis.json.GoogleJsonResponseException;
import com.google.api.client.googleapis.media.MediaHttpUploader;
import com.google.api.client.googleapis.media.MediaHttpUploaderProgressListener;
import com.google.api.client.http.HttpTransport;
import com.google.api.client.http.InputStreamContent;
import com.google.api.client.json.JsonFactory;
import com.google.api.client.json.gson.GsonFactory;
import com.google.api.services.youtube.YouTube;
import com.google.api.services.youtube.YouTubeScopes;
import com.google.api.services.youtube.model.Video;
import com.google.api.services.youtube.model.VideoSnippet;
import com.google.api.services.youtube.model.VideoStatus;
import com.unity3d.player.UnityPlayer;
import com.unity3d.player.UnityPlayerActivity;

/**
 * @author Oscar Crespo Salazar
 * @author Team Kite
 * 
 */

public class YoutubeUploader extends UnityPlayerActivity {
	
	private static YouTube youtube;
	
	private static final String TAG = "YoutubeUploader";
	
	private static final String OnCompleted = "OnCompleted";
	private static final String OnAuth = "OnAuth";
	private static final String OnCancelled = "OnCancelled";
	private static final String OnFailed = "OnFailed";
	
    private static final String VIDEO_FILE_FORMAT = "video/*";
    
    private static final int REQUEST_CODE = 1;
    
    public static YoutubeUploader activity;
    
    private static GoogleAccountCredential credential;
    
    public static final String[] SCOPES = {Scopes.PLUS_ME,YouTubeScopes.YOUTUBE_UPLOAD,YouTubeScopes.YOUTUBE_READONLY};
    
    public static final HttpTransport HTTP_TRANSPORT_DEFAULT = AndroidHttp.newCompatibleTransport();
    
    public static final JsonFactory JSON_FACTORY_DEFAULT = GsonFactory.getDefaultInstance();   
    
    private static String thisGameObjectCallBack;
    

    protected void onCreate(Bundle savedInstanceState) {
    	
		super.onCreate(savedInstanceState);
		activity = this;
    }

    public void onBackPressed(){
    }
  
    protected void onActivityResult(final int requestCode, final int resultCode, final Intent data) {
    	if (requestCode == REQUEST_CODE) {
    		if(resultCode == RESULT_OK){
	            String accountName = data.getStringExtra(AccountManager.KEY_ACCOUNT_NAME);
	            
	            
	            
	            AsyncTask<String, Void, Void> task = new AsyncTask<String, Void, Void>() {
	                @Override
	                protected Void doInBackground(String... params) {	
	                    getTokens(params[0]);
						return null;
	                }
	
	                @Override
	                protected void onPostExecute(Void token) {
	                    Log.i(TAG, "Access token retrieved:" + token);
	                }
	
	            };
	            task.execute(accountName);
    		}
    		else if(resultCode == RESULT_CANCELED){
    			UnityPlayer.UnitySendMessage(thisGameObjectCallBack, OnCancelled, "" );
    		}
    	}

    }
    public static void getTokens(String accountName){

        try {
        	    
        	credential = GoogleAccountCredential.usingOAuth2(activity,Arrays.asList(SCOPES));// Collections.singleton(YouTubeScopes.YOUTUBE_UPLOAD));
        	
        	credential.setSelectedAccountName(accountName);
        	
        	String tokens = credential.getToken();   
        	Log.i(TAG,"Tokens :"+tokens+ " callback "+thisGameObjectCallBack);
        	if(tokens != null){
        		UnityPlayer.UnitySendMessage(thisGameObjectCallBack, OnAuth, "" );
        	}
        	else{
        		UnityPlayer.UnitySendMessage(thisGameObjectCallBack, OnFailed, "Can't auth" );
        	}
        	

        }catch (GooglePlayServicesAvailabilityException playEx) {
        	Log.e(TAG,"GooglePlayServicesAvailabilityException authentication exception: " + playEx.getMessage());
        	UnityPlayer.UnitySendMessage(thisGameObjectCallBack, OnFailed, playEx.getMessage() );
        }catch (UserRecoverableAuthException recoverableException) {
        	activity.startActivityForResult(recoverableException.getIntent(), 0);  
        	UnityPlayer.UnitySendMessage(thisGameObjectCallBack, OnFailed,recoverableException.getMessage() );
        	Log.i(TAG,"UserRecoverableAuthException authentication exception: " + recoverableException.getMessage());
        } catch (GoogleAuthException authEx) {
            // This is likely unrecoverable.
        	UnityPlayer.UnitySendMessage(thisGameObjectCallBack, OnFailed, authEx.getMessage() );
        	Log.e(TAG,"Unrecoverable authentication exception: " + authEx.getMessage());      
        } catch (IOException e) {
        	Log.e(TAG,"IOException: " + e.getMessage());
        	UnityPlayer.UnitySendMessage(thisGameObjectCallBack, OnFailed, e.getMessage() );
        	e.printStackTrace();
        }
    }
    
    public static void authGoogle(String gameObjectCallBack){
    	thisGameObjectCallBack = gameObjectCallBack;
    	
    	Intent intent = AccountPicker.newChooseAccountIntent(null, null, new String[]{GoogleAuthUtil.GOOGLE_ACCOUNT_TYPE}, true, "", null, null, null);
    	
    	activity.startActivityForResult(intent, REQUEST_CODE);
    	
    }
    
    private static class BackgroundUpload extends AsyncTask<YouTube.Videos.Insert, Void, Void>{
    	 @Override
         protected Void doInBackground(YouTube.Videos.Insert... params) {
             try{          	
            	 YouTube.Videos.Insert videoInsert = params[0];
            	
            	 
            	 // Set the event listener.
                 MediaHttpUploader uploader = videoInsert.getMediaHttpUploader();

              
                 uploader.setDirectUploadEnabled(false);

                 MediaHttpUploaderProgressListener progressListener = new MediaHttpUploaderProgressListener() {
                     public void progressChanged(MediaHttpUploader uploader) throws IOException {
                         switch (uploader.getUploadState()) {
                             case INITIATION_STARTED:
                             	Log.i(TAG,"Initiation Started");
                                 break;
                             case INITIATION_COMPLETE:
                             	Log.i(TAG,"Initiation Completed");
                                 break;
                             case MEDIA_IN_PROGRESS:
                             	Log.i(TAG,"Upload in progress");
                             	Log.i(TAG,"Upload percentage: " + uploader.getProgress());
                                 break;
                             case MEDIA_COMPLETE:
                             	Log.i(TAG,"Upload Completed!");
                             	UnityPlayer.UnitySendMessage(thisGameObjectCallBack, OnCompleted, "" );
                                 break;
                             case NOT_STARTED:
                             	Log.i(TAG,"Upload Not Started!");
                                 break;
                         }
                     }
                 };
                 uploader.setProgressListener(progressListener);   	 
            	 
                 Video returnedVideo = videoInsert.execute();

                 Log.i(TAG,"  - Id: " + returnedVideo.getId());
                 Log.i(TAG,"  - Title: " + returnedVideo.getSnippet().getTitle());
                 Log.i(TAG,"  - Tags: " + returnedVideo.getSnippet().getTags());
                 Log.i(TAG,"  - Privacy Status: " + returnedVideo.getStatus().getPrivacyStatus());
                 Log.i(TAG,"  - Video Count: " + returnedVideo.getStatistics().getViewCount());

             } catch (GoogleJsonResponseException e) {
             	Log.e(TAG,"GoogleJsonResponseException code: " + e.getDetails().getCode() + " : " + e.getDetails().getMessage());
             	UnityPlayer.UnitySendMessage(thisGameObjectCallBack, OnFailed, e.getDetails().getMessage() );
                 e.printStackTrace();
             } catch (IOException e) {
             	Log.e(TAG,"IOException: " + e.getMessage());
             	UnityPlayer.UnitySendMessage(thisGameObjectCallBack, OnFailed, e.getMessage() );
                 e.printStackTrace();
             } catch (Throwable t) {
             	Log.e(TAG,"Throwable: " + t.getMessage());
             	UnityPlayer.UnitySendMessage(thisGameObjectCallBack, OnFailed, t.getMessage() );
                 t.printStackTrace();
             }	
             
             return null;
         }

         @Override
         protected void onPostExecute(Void result) {
         }

         @Override
         protected void onPreExecute() {
         }

         @Override
         protected void onProgressUpdate(Void... values) {
         }


    }
    
    public static void  uploadVideo(String path,String title, String description,String[] tag){  	
        try {
        	
    		youtube = new YouTube.Builder(HTTP_TRANSPORT_DEFAULT, JSON_FACTORY_DEFAULT, credential).setApplicationName("VideoUploader").build();
        	        	
            Log.i(TAG,"Uploading: " + path);

            Video videoObjectDefiningMetadata = new Video();

            
            VideoStatus status = new VideoStatus();
            status.setPrivacyStatus("public");
            videoObjectDefiningMetadata.setStatus(status);
           
            VideoSnippet snippet = new VideoSnippet();

            snippet.setTitle(title);
            snippet.setDescription(description);

            List<String> tags = new ArrayList<String>(Arrays.asList(tag));
         
            snippet.setTags(tags); 

            videoObjectDefiningMetadata.setSnippet(snippet);

            InputStreamContent mediaContent = new InputStreamContent(VIDEO_FILE_FORMAT,new FileInputStream(path));

            YouTube.Videos.Insert videoInsert = youtube.videos().insert("snippet,statistics,status", videoObjectDefiningMetadata, mediaContent);
            
          
            new BackgroundUpload().execute(videoInsert);


        } catch (GoogleJsonResponseException e) {
        	Log.e(TAG,"GoogleJsonResponseException code: " + e.getDetails().getCode() + " : " + e.getDetails().getMessage());
        	UnityPlayer.UnitySendMessage(thisGameObjectCallBack, OnFailed, e.getDetails().getMessage() );
            e.printStackTrace();
        } catch (IOException e) {
        	Log.e(TAG,"IOException: " + e.getMessage());
        	UnityPlayer.UnitySendMessage(thisGameObjectCallBack, OnFailed, e.getMessage() );
            e.printStackTrace();
        } catch (Throwable t) {
        	Log.e(TAG,"Throwable: " + t.getMessage());
        	UnityPlayer.UnitySendMessage(thisGameObjectCallBack, OnFailed, t.getMessage() );
            t.printStackTrace();
        }	
    	
    }
}