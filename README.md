# Unity ffmpeg-av-encoder

ffmpeg-av-encoder is a plugin to unity that allow you to record a video on Android witn openGLES 3.0 and upload it to youtube (iOS and Android).

This plugin use ffmpeg library.

How To
------
<b>FFMPEG video encoder</b>

The main utility of this software is the FFMPEG video encoder to unity (Android), to make it wotks properly you need a Android device with OpenGLES3.0 support.

The project is configured to eclipse (you need Android NDK & SDK and Eclipse CDT) and written in C.
The main file is: VideoEncoder/jni/VideoEncoder3.c.
To use it, just import the project: VideoEncoder

Also you will find an experimental code to iOS (not working) in: ffmpeg video encoder/

<b>Youtube</b>

The Android version is in: VideoEncoder
The iOS verion: YoutubeUploaderIOS


Requirements
------------
This software has been built for Android or iOS.

Authors
-------
Oscar Crespo Salazar

License
-------
The source code of this project is license under GPLv3

Copyright 2014 Team kite

For more information about License, please see file: LICENSE
