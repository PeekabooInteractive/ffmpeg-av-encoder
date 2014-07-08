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

//#define INPUT_AUDIO_FMT AV_SAMPLE_FMT_S16
//#define INPUT_AUDIO_FMT AV_SAMPLE_FMT_FLT
#define INPUT_AUDIO_FMT AV_SAMPLE_FMT_S16

//#define INPUT_AUDIO_RATE 44100
#define INPUT_AUDIO_RATE 24000

static uint8_t **src_samples_data;
volatile static int max_dst_nb_samples;
volatile static int src_nb_samples;
volatile static int src_samples_linesize;

uint8_t **dst_samples_data;
volatile int dst_samples_linesize;
volatile int dst_samples_size;
volatile int samples_count;

static float t, tincr, tincr2;

AVFrame *audio_frame;
struct SwrContext *swr_ctx = NULL;

pthread_mutex_t io_mutex;

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

void readBytesFromFile(uint8_t *inbuffer,char* path){

	//pthread_mutex_lock(&io_mutex);
	FILE *img = fopen(path,"rb");
	writeLog("filename %s\r\n", path);

	// obtain file size:
	fseek (img , 0 , SEEK_END);
	int lSize = ftell (img);
	rewind (img);

	// allocate memory to contain the whole file:
	//uint8_t *inbuffer = (uint8_t*) malloc (sizeof(uint8_t)* lSize);


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

	remove(path);

	//pthread_mutex_unlock(&io_mutex);
	//return inbuffer;
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
			c->sample_fmt = (*codec)->sample_fmts ? (*codec)->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
			//c->bit_rate = bit_rate;
			//c->sample_rate = 44100;
			//c->sample_rate = codec->supported_samplerates;


			/*if (!(*codec)->supported_samplerates){
				LOGE("NO");
				c->sample_rate = 48000;
			}
			else{
				LOGE("YES");
				int *p = (*codec)->supported_samplerates;
				int best_samplerate = 0;
				LOGE("While");
				while (*p) {
					best_samplerate = FFMAX(*p, best_samplerate);
					*p++;
				}
				c->sample_rate = best_samplerate;
			}*/

			c->sample_rate = 48000;
			c->bit_rate = 64000;
			c->channels = 2;
			c->channel_layout = AV_CH_LAYOUT_STEREO;

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

	//av_packet_rescale_ts(pkt, *time_base, st->time_base);

	pkt->stream_index = st->index;
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
		//LOGE("Could not open video codec: %s\n", av_err2str(ret));
		//write_log("Could not open video codec:\n");
		//exit(1);
	}
}

void converImageToEncode(AVPicture outpic,AVPicture inpic,uint8_t* inbuffer,int in_width,int in_height,struct SwsContext* sws_context){

	writeLog("Try to convert image");

	writeLog("Up to down initial image");
	int i;
	int j=0;
	for (i = in_height-1; i >= 0; i--){
		memcpy(&inpic.data[0][j*in_width * 3],&inbuffer[i*in_width * 3],in_width * 3);
		j++;
	}
	writeLog("Start scale");
	sws_scale(sws_context, (const uint8_t * const *)inpic.data, inpic.linesize, 0, in_height, outpic.data, outpic.linesize);
	writeLog("End scale");
}

volatile int count = 0;
void writeVideoFrame(AVFrame *frame,AVStream *video_st,AVFormatContext *formatContext){
	AVPacket pkt;

	av_init_packet(&pkt);

	pkt.data = NULL; // packet data will be allocated by the encoder
	pkt.size = 0;

	int got_output;

	//frame->pts = (1 / 25) *44100* count;
	int ret = avcodec_encode_video2(video_st->codec, &pkt, frame, &got_output);
	if (ret < 0) {
		LOGE("Error encoding frame %s\n",av_err2str(ret));
		writeLog("Error encoding frame\n");
		//exit(1);
	}
	if (got_output) {
		//write_log("Write frame  ");
		writeFrame(formatContext, &video_st->codec->time_base, video_st, &pkt);
	}
	av_free_packet(&pkt);
	count++;
	//count +=2;
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
	//src_nb_samples =  av_rescale_rnd(1024, c->sample_rate, INPUT_AUDIO_RATE , AV_ROUND_UP);//av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);//c->codec->capabilities & CODEC_CAP_VARIABLE_FRAME_SIZE ? 10000 : c->frame_size;
	//src_nb_samples =   1024;
	audio_frame->nb_samples = c->frame_size;
	src_nb_samples = c->frame_size;
	//ret = av_samples_alloc_array_and_samples(&src_samples_data, &src_samples_linesize, c->channels,src_nb_samples, AV_SAMPLE_FMT_S16, 0);
	ret = av_samples_alloc_array_and_samples(&src_samples_data, &src_samples_linesize, 2,src_nb_samples, INPUT_AUDIO_FMT, 0);
	if (ret < 0) {
		writeLog("Could not allocate source samples\n");
		exit(1);
	}
	/* compute the number of converted samples: buffering is avoided
	* ensuring that the output buffer will contain at least all the
	* converted input samples */
	//max_dst_nb_samples = src_nb_samples;
	//max_dst_nb_samples = av_rescale_rnd(src_nb_samples, c->sample_rate, c->sample_rate , AV_ROUND_UP);
	max_dst_nb_samples = c->frame_size;
	// LOGE("JODER %i %i",max_dst_nb_samples,src_nb_samples);
	//int64_t dst_ch_layout = AV_CH_LAYOUT_STEREO;
	//int64_t src_ch_layout = AV_CH_LAYOUT_STEREO;
	/* create resampler context */
	if (c->sample_fmt != INPUT_AUDIO_FMT) {

		swr_ctx = swr_alloc();
		if (!swr_ctx) {
			writeLog("Could not allocate resampler context\n");
			exit(1);
		}
		/* set options */
		//av_opt_set_int(swr_ctx, "in_channel_layout", src_ch_layout, 0);
		av_opt_set_int (swr_ctx, "in_channel_count", c->channels, 0);
		av_opt_set_int (swr_ctx, "in_sample_rate", c->sample_rate, 0);
		av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", INPUT_AUDIO_FMT, 0);

		//av_opt_set_int(swr_ctx, "out_channel_layout", dst_ch_layout, 0);
		av_opt_set_int (swr_ctx, "out_channel_count", c->channels, 0);
		av_opt_set_int (swr_ctx, "out_sample_rate", c->sample_rate, 0);
		av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", c->sample_fmt, 0);
		/* initialize the resampling context */
		if ((ret = swr_init(swr_ctx)) < 0) {
			writeLog("Failed to initialize the resampling context\n");
			exit(1);
		}
		ret = av_samples_alloc_array_and_samples(&dst_samples_data, &dst_samples_linesize, c->channels,max_dst_nb_samples, c->sample_fmt, 0);
		if (ret < 0) {
			writeLog("Could not allocate destination samples\n");
			exit(1);
		}
	}
	else {
		dst_samples_data = src_samples_data;
	}

	dst_samples_size = av_samples_get_buffer_size(NULL, c->channels, max_dst_nb_samples,c->sample_fmt, 0);

	//dst_samples_size = av_samples_get_buffer_size( &dst_samples_linesize, c->channels, ret,c->sample_fmt, 0);
}

void fillAudioSamples(int16_t* dataSource,uint8_t* samplesToFill,int channels){
	int j, i;
	int16_t *q;
	q = samplesToFill;
	for (j = 0; j < src_nb_samples; j++) {

		for (i = 0; i < channels; i++){
			*q++ = dataSource[j];
		}
	}

}

void writeAudioFrame(AVFormatContext *oc, AVStream *st,int16_t* data){

	AVCodecContext *c;
	AVPacket pkt = { 0 }; // data and size must be 0;
	int got_packet, ret, dst_nb_samples;
	av_init_packet(&pkt);
	c = st->codec;

	fillAudioSamples(data,src_samples_data[0],c->channels);

	if (swr_ctx) {
		/* compute destination number of samples */
		//dst_nb_samples = av_rescale_rnd(swr_get_delay(swr_ctx, c->sample_rate) + src_nb_samples,c->sample_rate, c->sample_rate, AV_ROUND_UP);
		dst_nb_samples = max_dst_nb_samples;
		/*if (dst_nb_samples > max_dst_nb_samples) {
			av_free(dst_samples_data[0]);
			ret = av_samples_alloc(dst_samples_data, &dst_samples_linesize, c->channels,dst_nb_samples, c->sample_fmt, 0);

			if (ret < 0){
				exit(1);
			}


			max_dst_nb_samples = dst_nb_samples;
			dst_samples_size = av_samples_get_buffer_size(NULL, c->channels, dst_nb_samples,c->sample_fmt, 0);
			//dst_samples_size = av_samples_get_buffer_size(&dst_samples_linesize, c->channels, ret,c->sample_fmt, 0);
		}
*/
		/* convert to destination format */
		ret = swr_convert(swr_ctx,dst_samples_data, dst_nb_samples,(const uint8_t **)src_samples_data, src_nb_samples);

		if (ret < 0) {
			writeLog("Error while converting\n");
			exit(1);
		}
	}
	else {
		dst_nb_samples = src_nb_samples;
	}


	audio_frame->pts = av_rescale_q(samples_count, (AVRational){1, c->sample_rate}, c->time_base);

	ret = avcodec_fill_audio_frame(audio_frame, c->channels, c->sample_fmt,dst_samples_data[0], dst_samples_size, 0);
	samples_count += dst_nb_samples;


	if (ret < 0) {
		//LOGE("Error avcodec_fill_audio_frame: %s\n", av_err2str(ret));
		writeLog("Error avcodec_fill_audio_frame: %s\n", av_err2str(ret));
		exit(1);
	}

	ret = avcodec_encode_audio2(c, &pkt, audio_frame, &got_packet);
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


/* Prepare a 16 bit dummy audio frame of 'frame_size' samples and
* 'nb_channels' channels. */
void getAudioFrame(int16_t *samples, int frame_size, int nb_channels){

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

void writeAudioFrameAutoGenerate(AVFormatContext *oc, AVStream *st, int flush){

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
			dst_nb_samples = av_rescale_rnd(swr_get_delay(swr_ctx, c->sample_rate) + src_nb_samples,c->sample_rate, c->sample_rate, AV_ROUND_UP);
			if (dst_nb_samples > max_dst_nb_samples) {
				av_free(dst_samples_data[0]);
				ret = av_samples_alloc(dst_samples_data, &dst_samples_linesize, c->channels,dst_nb_samples, c->sample_fmt, 0);

				if (ret < 0){
					LOGE("ERROR");
					//exit(1);
				}


				max_dst_nb_samples = dst_nb_samples;
				dst_samples_size = av_samples_get_buffer_size(NULL, c->channels, dst_nb_samples,c->sample_fmt, 0);
			}

			/* convert to destination format */
			ret = swr_convert(swr_ctx,dst_samples_data, dst_nb_samples,(const uint8_t **)src_samples_data, src_nb_samples);

			if (ret < 0) {
				writeLog("Error while converting\n");
				LOGE("Error while converting\n");
				exit(1);
			}
		}
		else {
			dst_nb_samples = src_nb_samples;
		}

		audio_frame->nb_samples = dst_nb_samples;
		audio_frame->pts = av_rescale_q(samples_count, (AVRational){1, c->sample_rate}, c->time_base);
		avcodec_fill_audio_frame(audio_frame, c->channels, c->sample_fmt,dst_samples_data[0], dst_samples_size, 0);
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

			writeAudioFrameAutoGenerate(formatContext, audio_st,0);
		}
		else if (video_st && video_time < audio_time) {
			pDirent = pDirs[i];
			sprintf(aux_path,"%s/%s",path,pDirent->d_name);

			//writeVideoFrameFromFile(aux_path,video_st,formatContext,in_width,in_height,out_width,out_height,sws_context);

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

//-----------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------MAIN PROGRAM--------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------------------

#define NUMR_PBO 5
GLuint m_pbos[NUMR_PBO];
int current_buffer;
int last_buffer;

volatile int current_image_encode;
volatile int current_image_read;

int x;
int y;
int in_width;
int in_height;
int out_width;
int out_height;
char* video_path;
int bit_rate;

pthread_t encode_thread;

volatile int exit_thread;
volatile int record;

volatile int thread_finished;

uint8_t *bytes;

volatile int size;

#define NUM_IMAGE 200
#define NUM_THREAD 2
struct struc_args{
	uint8_t* arg0[NUM_IMAGE];
	volatile int arg1;
};

struct struc_args* args_buffer[NUM_THREAD];

int turn_picture[NUM_THREAD];
volatile int lapPicture;

#define NUM_PICTURE NUM_THREAD*NUM_IMAGE
volatile AVPicture pictures[NUM_PICTURE];

#define NUM_SAMPLES 800
int16_t* audio_samples[NUM_SAMPLES];
volatile int lapSamples;

volatile int currente_sample_read;
volatile int currente_sample_encode;

volatile int finish_ini_encode_thread;

int num_copied_samples = 0;

void* encodeThread(){
	AVStream *audio_st, *video_st;
	//OutputStream video_st = { 0 }, audio_st = { 0 };
	AVFormatContext *formatContext;

	AVCodec *audio_codec, *video_codec;

	av_register_all();

	avformat_alloc_output_context2(&formatContext, NULL, NULL, video_path);
	if (!formatContext) {
		writeLog("Could not deduce output format from file extension: using MPEG.\n");
		avformat_alloc_output_context2(&formatContext, NULL, "mpeg", video_path);
	}
	if (!formatContext){
		//return 1;
	}

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

	av_dump_format(formatContext, 0, video_path, 1);
	//int out_width, out_height;

	int ret;
	if (!(format->flags & AVFMT_NOFILE)) {
		ret = avio_open(&formatContext->pb, video_path, AVIO_FLAG_WRITE);
		if (ret < 0) {
			writeLog("Could not open '%s': %s\n", video_path,av_err2str(ret));
			//write_log("Could not open");
			//return 1;
		}
	}

	ret = avformat_write_header(formatContext, NULL);
	if (ret < 0) {
		writeLog("Error occurred when opening output file: %s\n",av_err2str(ret));
		//write_log("Error occurred when opening output file");
		//return 1;
	}

	AVFrame* outframe;
	outframe = av_frame_alloc();

	AVPicture dataPicture;
	current_image_encode = 0;

	int turn = 0;

	int16_t* dataSamples;

	double audio_time, video_time;

	finish_ini_encode_thread = 1;

	audio_time = 0;
	video_time = 0;

	lapSamples = 0;

	while(exit_thread != 1 ){
		audio_time = audio_st->pts.val * av_q2d(audio_st->time_base);
		video_time = video_st->pts.val * av_q2d(video_st->time_base);

		if (audio_time <= video_time){
			if((currente_sample_encode < currente_sample_read && lapSamples == 0) || lapSamples == 1){

				dataSamples = audio_samples[currente_sample_encode];
				writeAudioFrame(formatContext, audio_st,dataSamples);

				free(dataSamples);
				audio_samples[currente_sample_encode] = NULL;

				currente_sample_encode++;

				if(currente_sample_encode >= NUM_SAMPLES){
					currente_sample_encode = 0;
					lapSamples=0;
				}

			}
		}
		else{
			if((current_image_encode <= turn_picture[turn] && lapPicture == 0) || lapPicture == 1){

				writeLog("Write frame");

				dataPicture = pictures[current_image_encode];

				*((AVPicture *)outframe) = dataPicture;
				outframe->pts = av_frame_get_best_effort_timestamp(outframe);

				writeVideoFrame(outframe,video_st,formatContext);

				avpicture_free(&dataPicture);

				current_image_encode++;
				if(current_image_encode >= NUM_PICTURE){
					current_image_encode = 0;
					lapPicture = 0;
				}

				turn = current_image_encode % NUM_THREAD;

			}
		}
	}

	while((currente_sample_encode < currente_sample_read && lapSamples == 0) || lapSamples == 1){

		free(audio_samples[currente_sample_encode]);

		currente_sample_encode++;

		if(currente_sample_encode >= NUM_SAMPLES){
			currente_sample_encode = 0;
			lapSamples=0;
		}
	}

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

	//av_free(inpic.data[0]);
	//av_free(outpic.data[0]);

	av_frame_free(&outframe);

	thread_finished = 1;

	writeLog("End thread encode");

	return NULL;
}

void* formatImage(void* args){

	int pos = 0;

	int id = (int)args;

	int pos_pictu = id;

	int turn;
	uint8_t *bytes;

	struct SwsContext* sws_context = sws_getContext(in_width, in_height, INPUT_PIX_FMT, out_width, out_height,STREAM_PIX_FMT, SWS_BICUBIC, NULL, NULL, NULL);

	AVPicture inpic;
	AVPicture outpic;

	AVPicture aux;

	avpicture_alloc(&inpic, INPUT_PIX_FMT, in_width, in_height);
	avpicture_alloc(&outpic, STREAM_PIX_FMT, out_width, out_height);

	lapPicture = 0;


	while(!exit_thread){
		turn = args_buffer[id]->arg1;

		if(pos != turn){

			//LOGE("The Turn is %i  of Thread   %i  NUM %i",turn,id,pos_pictu);

			bytes = args_buffer[id]->arg0[pos];

			converImageToEncode(outpic,inpic,bytes,in_width,in_height,sws_context);

			avpicture_alloc(&aux, STREAM_PIX_FMT, out_width, out_height);
			av_picture_copy(&aux,&outpic, STREAM_PIX_FMT, out_width, out_height);

			pictures[pos_pictu] = aux;
			turn_picture[id] = pos_pictu;

			pos_pictu+=NUM_THREAD;

			if(pos_pictu >= NUM_PICTURE){
				pos_pictu = id;
				lapPicture = 1;
			}

			pos++;

			if(pos >= NUM_IMAGE){
				pos = 0;
			}

			free(bytes);
		}

	}

	avpicture_free(&inpic);
	avpicture_free(&outpic);

	writeLog("FIN %i",id);


	return NULL;
}



void iniOpenGL(){

	memset(m_pbos, 0, sizeof(m_pbos));

	if (m_pbos[0] == 0){
		glGenBuffers(NUMR_PBO, m_pbos);
	}
	// create empty PBO buffers
	int i;
	for (i=0; i<NUMR_PBO; i++)
	{
		glBindBuffer(GL_PIXEL_PACK_BUFFER, m_pbos[i]);
		glBufferData(GL_PIXEL_PACK_BUFFER, size, NULL, GL_DYNAMIC_READ);
	}

	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

	// backbuffer to vram pbo index
	current_buffer = NUMR_PBO-1;
	last_buffer = 0;

}



void ini(int aux_x, int aux_y,int aux_in_width,int aux_in_height,int aux_out_width,int aux_out_height,char* aux_path,int aux_bit_rate){
	x= aux_x;
	y = aux_y;
	in_width = aux_in_width;
	in_height = aux_in_height;

	out_width = aux_out_width;
	out_height = aux_out_height;

	video_path = (char*)malloc(256*sizeof(char));

	strcpy(video_path,aux_path);

	bit_rate = aux_bit_rate;

	size = BITS_PER_PIXEL*in_width*in_height;


	exit_thread = 0;
	record = 0;
	thread_finished = 0;

	current_image_encode = 0;
	current_image_read = 0;

	currente_sample_read = 0;
	currente_sample_encode = 0;

	samples_count = 0;

	num_copied_samples = 0;

	iniOpenGL();

	memset(turn_picture,-1,sizeof(turn_picture));

	memset(audio_samples,NULL,sizeof(audio_samples));

	finish_ini_encode_thread = 0;
	//LOGE("SHIT1 %i",finish_ini_encode_thread);
	pthread_create(&encode_thread,NULL,encodeThread,NULL);



	iniThreads();

	while(finish_ini_encode_thread != 1){
		//LOGE("SHIT2 %i",finish_ini_encode_thread);
	}
}

void iniThreads(){
	pthread_t save_thread;
	int i;
	for(i = 0; i< NUM_THREAD;i++){
		args_buffer[i] = malloc(sizeof(struct struc_args));
		args_buffer[i]->arg1 = 0;
		pthread_create(&save_thread,NULL,formatImage,(void*)i);
	}
}

void recordVideo(){

	writeLog("BEGIN");
	glBindBuffer(GL_PIXEL_PACK_BUFFER, m_pbos[current_buffer]);
	writeLog("glBindBuffer");

	glReadPixels(x,y,in_width, in_height, GL_RGB, GL_UNSIGNED_BYTE, 0);

	glBindBuffer(GL_PIXEL_PACK_BUFFER, m_pbos[last_buffer]);

	writeLog("glReadPixels");
	GLubyte *ptr = glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, size, GL_MAP_READ_BIT);
	writeLog("glMapBufferRange");

	bytes = (uint8_t*)malloc(size*sizeof(uint8_t));

	memcpy(bytes, ptr, size);

	int id  = current_image_read % NUM_THREAD;
	int current = args_buffer[id]->arg1;
	args_buffer[id]->arg0[current] = bytes;


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


	args_buffer[id]->arg1++;

	if(args_buffer[id]->arg1 >= NUM_IMAGE){
		args_buffer[id]->arg1 = 0;
	}

	current_image_read++;

	if(current_image_read >= NUM_PICTURE){
		current_image_read = 0;
	}


	writeLog("RECORD");

}

int16_t* convertToS16(float* data,int size){
	int16_t *convertedData = malloc(size*sizeof(int16_t));

	int rescaleFactor = 32767;
	int i;
	for (i = 0; i < size;i++){

		convertedData[i] = (int16_t)(data[i]*rescaleFactor);
	}
	return convertedData;
}


void recordSample(float* dataSource,int size){
	int16_t* data = convertToS16(dataSource,size);

	int nb_sample = src_nb_samples;
	if(audio_samples[currente_sample_read] == NULL){
		audio_samples[currente_sample_read] = malloc(nb_sample*sizeof(int16_t));
		memset(audio_samples[currente_sample_read], 0, nb_sample*sizeof(int16_t));
	}

	int num_to_copy;
	int pos = num_copied_samples;
	num_copied_samples += size;

	if(num_copied_samples <= nb_sample){
		memcpy(&audio_samples[currente_sample_read][pos],data,size*sizeof(int16_t));
	}
	else {
		pos = num_copied_samples - size;
		num_copied_samples = num_copied_samples - nb_sample;

		int num = nb_sample - pos;

		memcpy(&audio_samples[currente_sample_read][pos],&data[0],num*sizeof(int16_t));

		currente_sample_read++;

		if(currente_sample_read >= NUM_SAMPLES){
			currente_sample_read = 0;
			lapSamples = 1;
		}

		audio_samples[currente_sample_read] = malloc(nb_sample*sizeof(int16_t));
		memset(audio_samples[currente_sample_read], 0,nb_sample*sizeof(int16_t));

		memcpy(&audio_samples[currente_sample_read][0],&data[num],num_copied_samples*sizeof(int16_t));
	}

	if(num_copied_samples == src_nb_samples){
		currente_sample_read++;
		num_copied_samples = 0;

		if(currente_sample_read >= NUM_SAMPLES){
			currente_sample_read = 0;
			lapSamples=1;
		}
	}

}

float freeMemory(){
	writeLog("FREE encode image: %i read image: %i encode sample: %i readl sample: %i",current_image_encode,current_image_read,currente_sample_encode,currente_sample_read);
	if(current_image_read != current_image_encode){
		return (float)((float)current_image_encode/(float)current_image_read);
	}

	writeLog("IT's finish");
	exit_thread = 1;

	if(currente_sample_read != currente_sample_encode){
		return 0.9f;
	}

	if(thread_finished == 0){
		return 0.9f;
	}

	pthread_join(encode_thread,NULL);
	//pthread_join(save_thread,NULL);

	free(video_path);

	//free(bytes);

	writeLog("END");
	return 1;
}

