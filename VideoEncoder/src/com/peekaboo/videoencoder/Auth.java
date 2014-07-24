package com.peekaboo.videoencoder;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.Reader;
import java.util.List;

import android.util.Log;

import com.google.api.client.auth.oauth2.Credential;
import com.google.api.client.auth.oauth2.StoredCredential;
import com.google.api.client.extensions.android.http.AndroidHttp;
import com.google.api.client.extensions.java6.auth.oauth2.AuthorizationCodeInstalledApp;
import com.google.api.client.extensions.jetty.auth.oauth2.LocalServerReceiver;
import com.google.api.client.googleapis.auth.oauth2.GoogleAuthorizationCodeFlow;
import com.google.api.client.googleapis.auth.oauth2.GoogleClientSecrets;
import com.google.api.client.http.HttpTransport;
import com.google.api.client.http.javanet.NetHttpTransport;
import com.google.api.client.json.JsonFactory;
import com.google.api.client.json.gson.GsonFactory;
import com.google.api.client.json.jackson2.JacksonFactory;
import com.google.api.client.util.store.DataStore;
import com.google.api.client.util.store.FileDataStoreFactory;





/**
 * Shared class used by every sample. Contains methods for authorizing a user and caching credentials.
 */
public class Auth {

    /**
     * Define a global instance of the HTTP transport.
     */
    public static final HttpTransport HTTP_TRANSPORT = new NetHttpTransport();
    
    public static final HttpTransport HTTP_TRANSPORT_DEFAULT = AndroidHttp.newCompatibleTransport();

    /**
     * Define a global instance of the JSON factory.
     */
    public static final JsonFactory JSON_FACTORY = new JacksonFactory();
    
    public static final JsonFactory JSON_FACTORY_DEFAULT = GsonFactory.getDefaultInstance();

    /**
     * This is the directory that will be used under the user's home directory where OAuth tokens will be stored.
     */
    private static final String CREDENTIALS_DIRECTORY = ".oauth-credentials";
    
    

    /**
     * Authorizes the installed application to access user's protected data.
     *
     * @param scopes              list of scopes needed to run youtube upload.
     * @param credentialDatastore name of the credential datastore to cache OAuth tokens
     */
    public static Credential authorize(List<String> scopes, String credentialDatastore,InputStream stream, String path ) throws IOException {
    	Log.e("VideoUploader", "DONE1");
        // Load client secrets.
    	//InputStream aux = Auth.class.getClassLoader().getResourceAsStream("/client_secrets.json");
    	//InputStream aux = Auth.class.getClassLoader()
    	if(stream == null){
    		Log.e("VideoUploader", "NULL");
    	}
    	Log.e("VideoUploader", "DONE1");
        Reader clientSecretReader = new InputStreamReader(stream);//Auth.class.getResourceAsStream("client_secrets.json"));
        Log.e("VideoUploader", "DONE2");
        
        GoogleClientSecrets clientSecrets = GoogleClientSecrets.load(JSON_FACTORY, clientSecretReader);
        Log.e("VideoUploader", "DONE3");
        // Checks that the defaults have been replaced (Default = "Enter X here").
        if (clientSecrets.getDetails().getClientId().startsWith("Enter")
                || clientSecrets.getDetails().getClientSecret().startsWith("Enter ")) {
        	Log.e("VideoUploader","Enter Client ID and Secret from https://code.google.com/apis/console/?api=youtube"+ "into src/main/resources/client_secrets.json");
            //System.exit(1);
        }

        // This creates the credentials datastore at ~/.oauth-credentials/${credentialDatastore}
        //FileDataStoreFactory fileDataStoreFactory = new FileDataStoreFactory(new File(System.getProperty("user.home") + "/" + CREDENTIALS_DIRECTORY));
        FileDataStoreFactory fileDataStoreFactory = new FileDataStoreFactory(new File(path + "/" + CREDENTIALS_DIRECTORY));
        Log.e("VideoUploader", "DONE4");
        DataStore<StoredCredential> datastore = fileDataStoreFactory.getDataStore(credentialDatastore);
        Log.e("VideoUploader", "DONE5");

        GoogleAuthorizationCodeFlow flow = new GoogleAuthorizationCodeFlow.Builder(HTTP_TRANSPORT, JSON_FACTORY, clientSecrets, scopes).setCredentialDataStore(datastore).build();
        Log.e("VideoUploader", "DONE6");
        // Build the local server and bind it to port 8080
        LocalServerReceiver localReceiver = new LocalServerReceiver.Builder().setPort(8080).build();
        Log.e("VideoUploader", "DONE7");
        // Authorize.
        
        
        return new AuthorizationCodeInstalledApp(flow, localReceiver).authorize("user");
    }
}
