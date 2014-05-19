//============================================================================
// Name        : example.cpp
// Author      : ocrespo
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C, Ansi-style
//============================================================================

#include <stdio.h>
//#include <stdlib.h>
#include <dirent.h>


#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
//#include "libswscale/version.h"

#include "libavutil/pixfmt.h"
#include "libavutil/opt.h"
//#include "libavutil/channel_layout.h"

#define GLES3 1

#if GLES3

#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <GLES3/gl3platform.h>

#else
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES2/gl2platform.h>

#endif


/*#include <GLES/gl.h>
#include <GLES/glext.h>
#include <GLES/glplatform.h>*/

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglplatform.h>

#include <time.h>

#include <wchar.h>

#include <jni.h>
#include <android/log.h>

#include <pthread.h>

#define DEBUG 0

#define PLATFORM_ANDROID 1
#define PLATFORM_IOS 0

#define LOG_TAG "VideoEncoder"
#define LOGI(...) __android_log_print(4, LOG_TAG, __VA_ARGS__);
#define LOGE(...) __android_log_print(6, LOG_TAG, __VA_ARGS__);



#define STREAM_FRAME_RATE 25 /* 25 images/s */
#define STREAM_PIX_FMT AV_PIX_FMT_YUV420P /* default pix_fmt */

#define BITS_PER_PIXEL 3

#define INPUT_PIX_FMT AV_PIX_FMT_RGB24
//#define INPUT_PIX_FMT AV_PIX_FMT_RGBA64

static uint8_t **src_samples_data;
static int max_dst_nb_samples;
static int src_nb_samples;
static int src_samples_linesize;

uint8_t **dst_samples_data;
int dst_samples_linesize;
int dst_samples_size;
int samples_count;

static float t, tincr, tincr2;

AVFrame *audio_frame;
struct SwrContext *swr_ctx = NULL;

/***********************************************************************************************************************************************
 * **********GENERAL****************************************************************************************************************************
 * *********************************************************************************************************************************************
 */

void writeLog(char *text,...){
	if(DEBUG){
#if PLATFORM_ANDROID
		LOGI("%s",text);
#elif PLATFORM_IOS
#endif
	}
}

void writeEGLError(char* id){

	EGLint error = eglGetError();
	if(error != EGL_SUCCESS){
		writeLog(id);
		//EGLint error = eglGetError();
		if(error == EGL_NO_SURFACE){
			writeLog("EGL_NO_SURFACE");
		}
		else if(error == EGL_BAD_DISPLAY){
			writeLog("EGL_BAD_DISPLAY");
		}
		else if(error == EGL_NOT_INITIALIZED){
			writeLog("EGL_NOT_INITIALIZED");
		}
		else if(error == EGL_BAD_CONFIG){
			writeLog("EGL_BAD_CONFIG");
		}
		else if(error == EGL_BAD_NATIVE_PIXMAP){
			writeLog("EGL_BAD_NATIVE_PIXMAP");
		}
		else if(error == EGL_BAD_ATTRIBUTE){
			writeLog("EGL_BAD_ATTRIBUTE");
		}
		else if(error == EGL_BAD_ALLOC){
			writeLog("EGL_BAD_ALLOC");
		}
		else if(error == EGL_BAD_MATCH){
			writeLog("EGL_BAD_MATCH");
		}
		else if(error == EGL_BAD_SURFACE){
			writeLog("EGL_BAD_SURFACE");
		}
		else if(error ==  EGL_CONTEXT_LOST ){
			writeLog(" EGL_CONTEXT_LOST ");
		}
		else {
			writeLog("ERROR NOT FOUND");
		}
	}
}

int dirCmp(const struct dirent  **first, const struct dirent **second){

	//PATH_MAX
	char file_1[256];
	char file_2[256];
	char *temp1 = file_1;
	char *temp2 = (char *)(*first)->d_name;

	// convert to upper case for comparison
	while (*temp2 != '\0'){
	  *temp1++ = toupper((int)*(temp2++));
	}

	temp1 = file_2;
	temp2 = (char *)(*second)->d_name;

	while (*temp2 != '\0'){
	  *temp1++ = toupper((int)*(temp2++));
	}

	return strcmp(file_1, file_2);
}
int dirSelect(const   struct   dirent   *dir){
	if (strcmp(".", dir->d_name) == 0 || strcmp("..", dir->d_name) == 0){
	  return 0;
	}
	else{
	  return 1;
   }
}

uint8_t* readBytesFromFile(char* path){
	FILE *img = fopen(path,"rb");
	writeLog("filename %s\r\n", path);

	// obtain file size:
	fseek (img , 0 , SEEK_END);
	int lSize = ftell (img);
	rewind (img);

	// allocate memory to contain the whole file:
	uint8_t *inbuffer = (uint8_t*) malloc (sizeof(uint8_t)* lSize);
	if (inbuffer == NULL) {
		fputs ("Memory error",stderr);
		exit (2);
	}

	// copy the file into the buffer:
	int result = fread (inbuffer,1,lSize,img);
	if (result != lSize) {
		fputs ("Reading error",stderr);
		exit (3);
	}
	fclose(img);
	return inbuffer;
}

AVStream* addStream(AVFormatContext *formatContext, AVCodec **codec,enum AVCodecID codec_id,int bit_rate,int out_width,int out_height){

	*codec = avcodec_find_encoder(codec_id);


	if (!(*codec)) {
		writeLog("Encode doesn't found \n");
		exit(-2);
	}

	AVStream *stream = avformat_new_stream(formatContext, *codec);
	if (!stream) {
		writeLog("Could not allocate stream\n");
		exit(1);
	}
	stream->id = formatContext->nb_streams-1;
	AVCodecContext *c = stream->codec;

	switch ((*codec)->type) {
		case AVMEDIA_TYPE_AUDIO:
			c->sample_fmt = (*codec)->sample_fmts ?
			(*codec)->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
			c->bit_rate = 64000;
			c->sample_rate = 44100;
			c->channels = 2;
			break;
		case AVMEDIA_TYPE_VIDEO:
			c->codec_id = codec_id;
			//c->bit_rate = 400000;
			c->bit_rate = bit_rate;
			/* Resolution must be a multiple of two. */
			c->width = out_width;
			c->height = out_height;

			/* timebase: This is the fundamental unit of time (in seconds) in terms
			* of which frame timestamps are represented. For fixed-fps content,
			* timebase should be 1/framerate and timestamp increments should be
			* identical to 1. */
			c->time_base.den = STREAM_FRAME_RATE;
			c->time_base.num = 1;
			c->gop_size = 12; /* emit one intra frame every twelve frames at most */
			c->pix_fmt = STREAM_PIX_FMT;
			if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
				/* just for testing, we also add B frames */
				c->max_b_frames = 2;
			}
			if (c->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
				/* Needed to avoid using macroblocks in which some coeffs overflow.
				* This does not happen with normal video, it just happens here as
				* the motion of the chroma plane does not match the luma plane. */
				c->mb_decision = 2;
			}
			break;
		default:
			break;
	}

	 /* Some formats want stream headers to be separate. */
	if (formatContext->oformat->flags & AVFMT_GLOBALHEADER){
		c->flags |= CODEC_FLAG_GLOBAL_HEADER;
	}

	return stream;
}


int writeFrame(AVFormatContext *fmt_ctx, const AVRational *time_base, AVStream *st, AVPacket *pkt){

	/* rescale output packet timestamp values from codec to stream timebase */
	pkt->pts = av_rescale_q_rnd(pkt->pts, *time_base, st->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
	pkt->dts = av_rescale_q_rnd(pkt->dts, *time_base, st->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
	pkt->duration = av_rescale_q(pkt->duration, *time_base, st->time_base);
	pkt->stream_index = st->index;
	/* Write the compressed frame to the media file. */
	//log_packet(fmt_ctx, pkt);
	//return av_write_frame(fmt_ctx,pkt);

	return av_interleaved_write_frame(fmt_ctx, pkt);
}

/***********************************************************************************************************************************************
 * **********VIDEO****************************************************************************************************************************
 * *********************************************************************************************************************************************
 */

void openVideo(AVFormatContext *oc, AVCodec *codec, AVStream *st){
	int ret;
	AVCodecContext *c = st->codec;

	ret = avcodec_open2(c, codec, NULL);
	if (ret < 0) {
		writeLog("Could not open video codec: %s\n", av_err2str(ret));
		//write_log("Could not open video codec:\n");
		exit(1);
	}
}


AVFrame* converImageToEncode(AVCodecContext* codec,uint8_t* inbuffer,int in_width,int in_height,int out_width,int out_height,struct SwsContext* sws_context){
	//int in_width, in_height, out_width, out_height;

	writeLog("Try to convert image");
	//here, make sure inbuffer points to the input BGR32 data,
	//and the input and output dimensions are set correctly.

	//calculate the bytes needed for the output image
	int nbytes = avpicture_get_size(codec->pix_fmt, out_width, out_height);

	//create ffmpeg frame structures.  These do not allocate space for image data,
	//just the pointers and other information about the image.
	AVFrame* inpic = av_frame_alloc();
	AVFrame* outpic = av_frame_alloc();

	//this will set the pointers in the frame structures to the right points in
	//the input and output buffers.
	//avpicture_alloc((AVPicture*)inpic, AV_PIX_FMT_BGR24, in_width, in_height);
	avpicture_alloc((AVPicture*)inpic, INPUT_PIX_FMT, in_width, in_height);
	writeLog("Up to down initial image");
	//avpicture_fill((AVPicture*)inpic, inbuffer, INPUT_PIX_FMT, in_width, in_height);

	int i;
	int j=0;
	for (i = in_height-1; i >= 0; i--){
		memcpy(&inpic->data[0][j*in_width * 3],&inbuffer[i*in_width * 3],in_width * 3);
		j++;
	}

	//create buffer for the output image
	uint8_t* outbuffer = (uint8_t*)av_malloc(nbytes*sizeof(uint8_t));

	writeLog("Fill initial image");

	avpicture_fill((AVPicture*)outpic, outbuffer, codec->pix_fmt, out_width, out_height);


	//return outpic;

	//create the conversion context
	//struct SwsContext* context = sws_getContext(in_width, in_height, PIX_FMT_BGR24, out_width, out_height, codec->pix_fmt, SWS_BICUBIC, NULL, NULL, NULL);

	//sws_getContext(width-x, height-y, INPUT_PIX_FMT, width, height,video_st->codec->pix_fmt, SWS_BICUBIC, NULL, NULL, NULL);

	//perform the conversion
	writeLog("Start scale");
	sws_scale(sws_context, (const uint8_t * const *)inpic->data, inpic->linesize, 0, in_height, outpic->data, outpic->linesize);
	writeLog("End scale");
	av_free(inpic);



	return outpic;
}


void writeVideoFrameFromFile(char* path,AVStream *video_st,AVFormatContext *formatContext,int in_width,int in_height,int out_width,int out_height,struct SwsContext* sws_context){
	AVPacket pkt;

	av_init_packet(&pkt);

	pkt.data = NULL; // packet data will be allocated by the encoder
	pkt.size = 0;

	uint8_t* inbuffer = readBytesFromFile(path);

	AVFrame* frame = converImageToEncode(video_st->codec,inbuffer,in_width,in_height,out_width,out_height,sws_context);

	int got_output;

	int ret = avcodec_encode_video2(video_st->codec, &pkt, frame, &got_output);
	if (ret < 0) {
		writeLog("Error encoding frame\n");
		exit(1);
	}
	if (got_output) {
		LOGE("Write frame  (size=%5d)\n", pkt.size);
		//write_log("Write frame  ");
		writeFrame(formatContext, &video_st->codec->time_base, video_st, &pkt);
	}

	av_free(frame);
	free(inbuffer);
}

volatile int turn_encode;

struct struc_args{
	void *arg0;
	void *arg1;
	void *arg2;
	void *arg3;
	void *arg4;
	void *arg5;
	void *arg6;
	void *arg7;
	void *arg8;
};

void* writeVideoFrame(void* args){
	struct struc_args *arg = args;
	uint8_t* inbuffer = arg->arg0;
	AVStream *video_st = arg->arg1;
	AVFormatContext *formatContext = arg->arg2;
	int in_width = *((int*)arg->arg3);
	int in_height = *((int*)arg->arg4);
	int out_width = *((int*)arg->arg5);
	int out_height = *((int*)arg->arg6);
	struct SwsContext* sws_context = arg->arg7;
	int id = *((int*)arg->arg8);


	AVPacket pkt;

	av_init_packet(&pkt);

	pkt.data = NULL; // packet data will be allocated by the encoder
	pkt.size = 0;

	AVFrame* frame = converImageToEncode(video_st->codec,inbuffer,in_width,in_height,out_width,out_height,sws_context);

	int got_output;
	if(id != turn_encode){
		sleep(1);
	}
	while(id != turn_encode){

	}

	LOGE("Encode %i",id);
	int ret = avcodec_encode_video2(video_st->codec, &pkt, frame, &got_output);
	if (ret < 0) {
		writeLog("Error encoding frame\n");
		exit(1);
	}
	LOGE("Ready to write %i",id);
	turn_encode++;
	if (got_output) {
		LOGE("Write frame  (size=%5d)\n", pkt.size);
		//write_log("Write frame\n");
		writeFrame(formatContext, &video_st->codec->time_base, video_st, &pkt);
		LOGE("END Write frame  (size=%5d)\n", pkt.size);
	}


	/*if(turn_encode > 30){
		turn_encode = 0;
	}*/

	av_free(frame);
	free(inbuffer);

	pthread_exit(1);
}

void closeVideo(AVFormatContext *oc, AVStream *st){

	avcodec_close(st->codec);

}

/***********************************************************************************************************************************************
 * **********AUDIO****************************************************************************************************************************
 * *********************************************************************************************************************************************
 */

void openAudio(AVFormatContext *oc, AVCodec *codec, AVStream *st)
{
	AVCodecContext *c;
	int ret;
	c = st->codec;
	/* allocate and init a re-usable frame */
	audio_frame = av_frame_alloc();
	if (!audio_frame) {
		writeLog("Could not allocate audio frame\n");
		exit(1);
	}
	/* open it */
	ret = avcodec_open2(c, codec, NULL);
	if (ret < 0) {
		writeLog( "Could not open audio codec: %s\n", av_err2str(ret));
		//write_log( "Could not open audio codec:\n");
		exit(1);
	}
	/* init signal generator */
	t = 0;
	tincr = 2 * M_PI * 110.0 / c->sample_rate;
	/* increment frequency by 110 Hz per second */
	tincr2 = 2 * M_PI * 110.0 / c->sample_rate / c->sample_rate;
	src_nb_samples = c->codec->capabilities & CODEC_CAP_VARIABLE_FRAME_SIZE ?
	10000 : c->frame_size;
	ret = av_samples_alloc_array_and_samples(&src_samples_data, &src_samples_linesize, c->channels,
	src_nb_samples, AV_SAMPLE_FMT_S16, 0);
	if (ret < 0) {
		writeLog("Could not allocate source samples\n");
		exit(1);
	}
	/* compute the number of converted samples: buffering is avoided
	* ensuring that the output buffer will contain at least all the
	* converted input samples */
	max_dst_nb_samples = src_nb_samples;
	/* create resampler context */
	if (c->sample_fmt != AV_SAMPLE_FMT_S16) {
		swr_ctx = swr_alloc();
		if (!swr_ctx) {
			writeLog("Could not allocate resampler context\n");
			exit(1);
		}
		/* set options */
		av_opt_set_int (swr_ctx, "in_channel_count", c->channels, 0);
		av_opt_set_int (swr_ctx, "in_sample_rate", c->sample_rate, 0);
		av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", AV_SAMPLE_FMT_S16, 0);
		av_opt_set_int (swr_ctx, "out_channel_count", c->channels, 0);
		av_opt_set_int (swr_ctx, "out_sample_rate", c->sample_rate, 0);
		av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", c->sample_fmt, 0);
		/* initialize the resampling context */
		if ((ret = swr_init(swr_ctx)) < 0) {
			writeLog("Failed to initialize the resampling context\n");
			exit(1);
		}
		ret = av_samples_alloc_array_and_samples(&dst_samples_data, &dst_samples_linesize, c->channels,
		max_dst_nb_samples, c->sample_fmt, 0);
		if (ret < 0) {
			writeLog("Could not allocate destination samples\n");
			exit(1);
		}
	}
	else {
		dst_samples_data = src_samples_data;
	}

	dst_samples_size = av_samples_get_buffer_size(NULL, c->channels, max_dst_nb_samples,
	c->sample_fmt, 0);
}



/* Prepare a 16 bit dummy audio frame of 'frame_size' samples and
* 'nb_channels' channels. */
static void getAudioFrame(int16_t *samples, int frame_size, int nb_channels){

	int j, i, v;
	int16_t *q;
	q = samples;
	for (j = 0; j < frame_size; j++) {
		v = (int)(sin(t) * 10000);
		for (i = 0; i < nb_channels; i++){
			*q++ = v;
		}

		t += tincr;
		tincr += tincr2;
	}
}

void writeAudioFrame(AVFormatContext *oc, AVStream *st, int flush){

	AVCodecContext *c;
	AVPacket pkt = { 0 }; // data and size must be 0;
	int got_packet, ret, dst_nb_samples;
	av_init_packet(&pkt);
	c = st->codec;
	if (!flush) {
		getAudioFrame((int16_t *)src_samples_data[0], src_nb_samples, c->channels);
		/* convert samples from native format to destination codec format, using the resampler */
		if (swr_ctx) {
			/* compute destination number of samples */
			dst_nb_samples = av_rescale_rnd(swr_get_delay(swr_ctx, c->sample_rate) + src_nb_samples,
			c->sample_rate, c->sample_rate, AV_ROUND_UP);
			if (dst_nb_samples > max_dst_nb_samples) {
			av_free(dst_samples_data[0]);
			ret = av_samples_alloc(dst_samples_data, &dst_samples_linesize, c->channels,
			dst_nb_samples, c->sample_fmt, 0);
			if (ret < 0)
			exit(1);
			max_dst_nb_samples = dst_nb_samples;
			dst_samples_size = av_samples_get_buffer_size(NULL, c->channels, dst_nb_samples,
			c->sample_fmt, 0);
		}

		/* convert to destination format */
		ret = swr_convert(swr_ctx,
		dst_samples_data, dst_nb_samples,
		(const uint8_t **)src_samples_data, src_nb_samples);

		if (ret < 0) {
			writeLog("Error while converting\n");
			exit(1);
		}
	}
		else {
			dst_nb_samples = src_nb_samples;
	}

	audio_frame->nb_samples = dst_nb_samples;
	audio_frame->pts = av_rescale_q(samples_count, (AVRational){1, c->sample_rate}, c->time_base);
	avcodec_fill_audio_frame(audio_frame, c->channels, c->sample_fmt,
	dst_samples_data[0], dst_samples_size, 0);
	samples_count += dst_nb_samples;
	}
	ret = avcodec_encode_audio2(c, &pkt, flush ? NULL : audio_frame, &got_packet);
	if (ret < 0) {
		writeLog("Error encoding audio frame: %s\n", av_err2str(ret));
		//write_log("Error encoding audio frame\n");
		exit(1);
	}
	if (!got_packet) {
		//if (flush)
			//audio_is_eof = 1;
		return;
	}
	ret = writeFrame(oc, &c->time_base, st, &pkt);
	if (ret < 0) {
		writeLog("Error while writing audio frame: %s\n",av_err2str(ret));
		//write_log("Error while writing audio frame\n");
		exit(1);
	}
}

void closeAudio(AVFormatContext *oc, AVStream *st){

	avcodec_close(st->codec);

	if (dst_samples_data != src_samples_data) {
		av_free(dst_samples_data[0]);
		av_free(dst_samples_data);
	}

	av_free(src_samples_data[0]);
	av_free(src_samples_data);
	av_frame_free(&audio_frame);
}



/***********************************************************************************************************************************************
 * **********MAIN****************************************************************************************************************************
 * *********************************************************************************************************************************************
 */

int createVideoFromDirectory(char* path,char* out_file,int in_width,int in_height,int bit_rate,int out_width,int out_height){
	struct dirent *pDirent;
	struct  dirent  **pDirs;

	AVCodec *audio_codec, *video_codec;

	av_register_all();

	AVFormatContext *formatContext;
	avformat_alloc_output_context2(&formatContext, NULL, NULL, out_file);
	if (!formatContext) {
		writeLog("Could not deduce output format from file extension: using MPEG.\n");
		avformat_alloc_output_context2(&formatContext, NULL, "mpeg", out_file);
	}
	if (!formatContext)
		return 1;

	AVStream *audio_st, *video_st;

	AVOutputFormat *format = formatContext->oformat;
	/* Add the audio and video streams using the default format codecs
	* and initialize the codecs. */
	video_st = NULL;
	audio_st = NULL;
	if (format->video_codec != AV_CODEC_ID_NONE){
		video_st = addStream(formatContext, &video_codec, format->video_codec,bit_rate,out_width,out_height);
	}

	if (format->audio_codec != AV_CODEC_ID_NONE){
		audio_st = addStream(formatContext, &audio_codec, format->audio_codec,bit_rate,out_width,out_height);
	}

	 /* Now that all the parameters are set, we can open the audio and
	* video codecs and allocate the necessary encode buffers. */
	if (video_st){
		openVideo(formatContext, video_codec, video_st);
	}

	if (audio_st){
		openAudio(formatContext, audio_codec, audio_st);
	}


	//int out_width, out_height;

	int ret;
	if (!(format->flags & AVFMT_NOFILE)) {
		ret = avio_open(&formatContext->pb, out_file, AVIO_FLAG_WRITE);
		if (ret < 0) {
			writeLog("Could not open '%s': %s\n", out_file,av_err2str(ret));
			//write_log("Could not open\n");
			return 1;
		}
	}

	ret = avformat_write_header(formatContext, NULL);
	if (ret < 0) {
		writeLog("Error occurred when opening output file: %s\n",av_err2str(ret));
		//write_log("Error occurred when opening output file");
		return 1;
	}
	//out_width=video_st->codec->width;
	//out_height=video_st->codec->height;

	char* aux_path = (char*)malloc(256*sizeof(char));
	int i = 0;

	int dirNum = scandir(path,&pDirs,dirSelect,dirCmp);
	if(dirNum <= 0){
		writeLog( "could not open %s\r\n", path);
		//write_log( "could not open");
		return -1;
	}

	struct SwsContext* sws_context = sws_getContext(in_width, in_height, AV_PIX_FMT_BGR24, out_width, out_height,video_st->codec->pix_fmt, SWS_BICUBIC, NULL, NULL, NULL);

	double audio_time, video_time;
	while(dirNum > 0){
		 /* Compute current audio and video time. */
		audio_time = (audio_st) ? audio_st->pts.val * av_q2d(audio_st->time_base) : INFINITY;
		video_time = (video_st) ? video_st->pts.val * av_q2d(video_st->time_base) : INFINITY;

		 /* write interleaved audio and video frames */
		if (audio_st && audio_time <= video_time) {
			writeAudioFrame(formatContext, audio_st,0);
		}
		else if (video_st && video_time < audio_time) {
			pDirent = pDirs[i];
			sprintf(aux_path,"%s/%s",path,pDirent->d_name);

			writeVideoFrameFromFile(aux_path,video_st,formatContext,in_width,in_height,out_width,out_height,sws_context);

			i++;
			dirNum--;



		}
		if(i == 100){
			writeLog("500\n");
		}

	}
	writeLog("END ENCODE\n");

	sws_freeContext(sws_context);
	//closedir(pDirs);
	 av_write_trailer(formatContext);
	/* Close each codec. */
	if (video_st){
		closeVideo(formatContext, video_st);
	}

	if (audio_st){
		closeAudio(formatContext, audio_st);
	}

	if (!(format->flags & AVFMT_NOFILE)){
		/* Close the output file. */
		avio_close(formatContext->pb);
	}

	// free the stream
	avformat_free_context(formatContext);

	free(aux_path);
	writeLog("END \r\n");

	return 0;
}

int generateVideoFromImages(char* dir,char* out_file,int in_width,int in_height,int bit_rate,int out_width,int out_height) {
	return createVideoFromDirectory(dir,out_file,in_width,in_height,bit_rate,out_width,out_height);
	//return EXIT_SUCCESS;
}


int x;
int y;
int width;
int height;
char* path;

int bit_rate;

struct SwsContext* sws_context;

AVStream *audio_st, *video_st;
AVFormatContext *formatContext;

double audio_time, video_time;

pthread_t encode_thread;

volatile int exit_thread;
volatile int record;

volatile int finish_encode_frame;

volatile int thread_finished;

//uint8_t *bytes;
int size;

void ini(int aux_x, int aux_y,int aux_width,int aux_height,char* aux_path,int aux_bit_rate){
	x= aux_x;
	y = aux_y;
	width = aux_width;
	height = aux_height;
	path = (char*)malloc(256*sizeof(char));
	strcpy(path,aux_path);

	bit_rate = aux_bit_rate;

	iniAvCodec();

	exit_thread = 0;
	record = 0;
	thread_finished = 0;
	finish_encode_frame = 1;
	//ini_thread();
}

GLuint pbo_id;

#define NUMR_PBO 4
GLuint m_pbos[NUMR_PBO];
int gpu2vram;

/*#define NUMR_IMAGE 30
uint8_t* images[NUMR_IMAGE];
*/
volatile int current_image_encode;
volatile int current_image_read;

uint8_t *bytes;

GLuint rboId;  //render buffer id

void* encodeThread(){
	//finish_encode_frame = 1;


	finish_encode_frame = 1;
	uint8_t *data;

	int id ;
	//while(exit_thread != 1 || current_image_encode != current_image_read){
	while(exit_thread != 1 ){

		if(current_image_encode != current_image_read ){
			finish_encode_frame = 0;
			writeLog("Write frame");
			//data = images[current_image_encode];

			//writeVideoFrame(data,video_st,formatContext,width-x,height-y,width,height,sws_context);
			int aux_width = width-x;
			int aux_height = height-y;
			/*void* args[9] = {
					data,
					video_st,
					formatContext,
					&aux_width,
					&aux_height,
					&width,
					&height,
					sws_context,
					current_image_encode
			};*/
			id = current_image_encode;
			struct struc_args args;
			args.arg0 = bytes;
			args.arg1 = video_st;
			args.arg2 = formatContext;
			args.arg3 = &aux_width;
			args.arg4 = &aux_height;
			args.arg5 = &width;
			args.arg6 = &height;
			args.arg7 = sws_context;
			args.arg8 = &id;

			pthread_t thread;
			pthread_create(&thread,NULL,writeVideoFrame,&args);


			writeLog("End write frame");

			//free(data);
			//images[current_image_encode] = NULL;

			//LOGE("ENCODE %i",current_image_encode);

			current_image_encode++;
			/*if(current_image_encode > NUMR_IMAGE){
				current_image_encode = 0;
			}*/
			finish_encode_frame = 1;
		}




	}
	while(turn_encode != current_image_encode){
		sleep(1);
	}
	finish_encode_frame = 1;

	record = 0;



	thread_finished = 1;
}


int iniAvCodec(){
	AVCodec *audio_codec, *video_codec;

	av_register_all();

	//AVFormatContext *formatContext;
	avformat_alloc_output_context2(&formatContext, NULL, NULL, path);
	if (!formatContext) {
		writeLog("Could not deduce output format from file extension: using MPEG.\n");
		avformat_alloc_output_context2(&formatContext, NULL, "mpeg", path);
	}
	if (!formatContext)
		return 1;

	AVOutputFormat *format = formatContext->oformat;


	/* Add the audio and video streams using the default format codecs
	* and initialize the codecs. */
	video_st = NULL;
	audio_st = NULL;
	if (format->video_codec != AV_CODEC_ID_NONE){
		video_st = addStream(formatContext, &video_codec, format->video_codec,bit_rate,width,height);
	}

	if (format->audio_codec != AV_CODEC_ID_NONE){
		audio_st = addStream(formatContext, &audio_codec, format->audio_codec,bit_rate,width,height);
	}

	 /* Now that all the parameters are set, we can open the audio and
	* video codecs and allocate the necessary encode buffers. */
	if (video_st){
		openVideo(formatContext, video_codec, video_st);
	}

	if (audio_st){
		openAudio(formatContext, audio_codec, audio_st);
	}


	//int out_width, out_height;

	int ret;
	if (!(format->flags & AVFMT_NOFILE)) {
		ret = avio_open(&formatContext->pb, path, AVIO_FLAG_WRITE);
		if (ret < 0) {
			writeLog("Could not open '%s': %s\n", path,av_err2str(ret));
			//write_log("Could not open");
			return 1;
		}
	}

	ret = avformat_write_header(formatContext, NULL);
	if (ret < 0) {
		writeLog("Error occurred when opening output file: %s\n",av_err2str(ret));
		//write_log("Error occurred when opening output file");
		return 1;
	}
	//out_width=video_st->codec->width;
	//out_height=video_st->codec->height;

	/*char* aux_path = (char*)malloc(256*sizeof(char));
	int i = 0;*/

	//sws_context = sws_getContext(width, height, AV_PIX_FMT_BGR24, width, height,video_st->codec->pix_fmt, SWS_BICUBIC, NULL, NULL, NULL);
	sws_context = sws_getContext(width-x, height-y, INPUT_PIX_FMT, width, height,video_st->codec->pix_fmt, SWS_BICUBIC, NULL, NULL, NULL);

	size = BITS_PER_PIXEL*(width-x)*(height-y);


	memset(m_pbos, 0, sizeof(m_pbos));

	if (m_pbos[0] == 0){
		glGenBuffers(NUMR_PBO, m_pbos);
	}
	// create empty PBO buffers
	int i;
	for (i=0; i<NUMR_PBO; i++)
	{
		glBindBuffer(GL_PIXEL_PACK_BUFFER, m_pbos[i]);
		glBufferData(GL_PIXEL_PACK_BUFFER, size, NULL, GL_STATIC_READ);
	}

	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

	// backbuffer to vram pbo index
	gpu2vram = NUMR_PBO-1;

	current_image_read = 0;
	current_image_encode = 0;



	//memset(images,NULL,sizeof(images));

	pthread_create(&encode_thread,NULL,encodeThread,NULL);

	exit_thread = 0;

	return 0;
}


/*void createContext(){
	eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	write_EGL_error("eglGetDisplay");


	GLuint major;
	GLuint minor;
	if(!eglInitialize(eglDisplay,&major,&minor)){
		write_EGL_error("eglGetCurrentContext");
	}


	EGLContext eglMainContext = eglGetCurrentContext();
	write_EGL_error("eglGetCurrentContext");

	EGLint contextAttributes[] = {
		EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_RED_SIZE, 5,
		EGL_GREEN_SIZE, 6,
		EGL_BLUE_SIZE, 5,
		EGL_DEPTH_SIZE, 1,
		EGL_NONE
	};
	const EGLint maxConfigs = 10;
	EGLConfig configs[maxConfigs]; // We'll only accept 10 configs
	EGLint num_config;
	if(!eglChooseConfig(eglDisplay,contextAttributes,configs,maxConfigs,&num_config)){
		write_EGL_error("eglChooseConfig");
	}

	EGLint attribList[] = {
		EGL_WIDTH, width,
		EGL_HEIGHT, height,
		EGL_LARGEST_PBUFFER, EGL_TRUE,
		EGL_NONE
	};
	eglSurface1 = eglCreatePbufferSurface(eglDisplay, configs[0], attribList);
	if(eglSurface1 == EGL_NO_SURFACE){
		write_EGL_error("eglCreatePixmapSurface");
	}

	EGLint attribLista[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};

	EGLContext context = eglGetCurrentContext();
	eglContext1 = eglCreateContext(eglDisplay, configs[0], context, attribLista);
	write_EGL_error("eglCreateContext");


	if(!eglMakeCurrent(eglDisplay, eglSurface1, eglSurface1, eglContext1)){
		write_EGL_error("eglMakeCurrent");
	}

	eglMainContext = eglGetCurrentContext();
	if(eglMainContext == eglContext1){
		write_log("YEAH FUCKING SAME");
	}
}*/
void* encodeImage(void* bytes){




	//free(bytes);


}

void recordVideo(){
	/*if(images[current_image_read] != NULL){
		writeLog("NO IMAGE");
		return;
	}*/

	//while(finish_encode_frame != 1){
		//Sleep();
	//}
	//glReadBuffer(GL_COLOR_ATTACHMENT0);
	writeLog("BEGIN");
	glBindBuffer(GL_PIXEL_PACK_BUFFER, m_pbos[gpu2vram]);
	writeLog("glBindBuffer");

	glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, 0);
	writeLog("glReadPixels");
	GLubyte *ptr = glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, size, GL_MAP_READ_BIT);
	writeLog("glMapBufferRange");

	//LOGE("READ: %i",current_image_read);
	bytes = (uint8_t*)malloc(size*sizeof(uint8_t));
	//LOGE("RECORD");
	memcpy(bytes, ptr, size);
	//images[current_image_read] = bytes;

	/*if(current_image_read > NUMR_IMAGE){
		current_image_read = 0;
	}*/

	current_image_read++;

	//ini_thread();
	//record = 1;
	//pthread_create(&encode_thread,NULL,encodeImage,(void*)bytes);

	glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
	writeLog("glUnmapBuffer");
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
	writeLog("glBindBuffer");

	// shift names
	GLuint temp = m_pbos[0];
	int i;
	for (i=1; i<NUMR_PBO; i++){
		m_pbos[i-1] = m_pbos[i];
	}
	m_pbos[NUMR_PBO - 1] = temp;

	writeLog("RECORD");
}

void freeMemory(){

	exit_thread = 1;
	record = 0;

	while(thread_finished == 0){
		sleep(1);
	}
	/*while(turn_encode != current_image_encode){
		sleep(1);
	}*/


	sws_freeContext(sws_context);
	//closedir(pDirs);
	 av_write_trailer(formatContext);
	/* Close each codec. */
	if (video_st){
		closeVideo(formatContext, video_st);
	}

	if (audio_st){
		closeAudio(formatContext, audio_st);
	}

	if (!(formatContext->oformat->flags & AVFMT_NOFILE)){
		/* Close the output file. */
		avio_close(formatContext->pb);
	}

	// free the stream
	avformat_free_context(formatContext);

	free(path);

	//free(bytes);
}

