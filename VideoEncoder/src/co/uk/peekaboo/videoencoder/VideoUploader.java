package co.uk.peekaboo.videoencoder;

import com.unity3d.player.UnityPlayerActivity;
import android.os.Bundle;
import android.util.Log;

public class VideoUploader extends UnityPlayerActivity {

  protected void onCreate(Bundle savedInstanceState) {

    // call UnityPlayerActivity.onCreate()
    super.onCreate(savedInstanceState);

    // print debug message to logcat
    Log.d("OverrideActivity", "onCreate called!");
  }

  public void onBackPressed()
  {
    // instead of calling UnityPlayerActivity.onBackPressed() we just ignore the back button event
    // super.onBackPressed();
  }
  
}