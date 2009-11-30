
#include <jni.h>
#include <stdlib.h>
#include <math.h>
#include <android/log.h>
#include "importgl.h"

#define LOG_TAG "libiteration"

#define TABLE_BITS (12)
#define TABLE_SIZE (1<<TABLE_BITS)

#define MAX(a,b) ((a)>(b))?(a):(b)
#define MIN(a,b) ((a)<(b))?(a):(b)

static int sin_table[TABLE_SIZE];

static int windowWidth = 320;
static int windowHeight = 480;

static int textureWidth = 512;
static int textureHeight = 512;

static int targetWidth = 320;
static int targetHeight = 480;

static int vert_coords[8];
static int tex_coords[8];

static jfieldID
	FID_niters,
	FID_ncps,

	FID_width,
	FID_height,

	FID_xc,
	FID_yc,
	FID_xs,
	FID_ys,

	FID_a,
	FID_b,
	FID_c,
	FID_d,

	FID_vt,

	FID_invert,

	FID_colorLut,
	FID_fancyBuf,
	FID_targetBuf;

static int makePow2(int in) {
	int out = 1;
	while(in > 0) {
		in >>= 1;
		out <<= 1;
	}

	return out;
}

void Java_as_adamsmith_iteration_SketchRenderer_nativeInit(JNIEnv* env, jobject ths) {

	importGLInit();

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FIXED, 0, vert_coords);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FIXED, 0, tex_coords);


	int texId;
	glGenTextures(1,&texId);
	glBindTexture(GL_TEXTURE_2D, texId);
	glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);



	glEnable(GL_TEXTURE_2D);

	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE);
}

void Java_as_adamsmith_iteration_SketchRenderer_nativeResize(
		JNIEnv* env,
		jobject ths,
		jint w,
		jint h,
		jboolean supersine) {

	__android_log_print(ANDROID_LOG_INFO, LOG_TAG, "resize w=%d h=%d", w, h);

	const jobject jtargetBuf = (*env)->GetObjectField(env, ths, FID_targetBuf);
	int* targetBuf = (*env)->GetDirectBufferAddress(env, jtargetBuf);

	const int invert = (*env)->GetBooleanField(env, ths, FID_invert);
			
	targetWidth = (*env)->GetIntField(env, ths, FID_width);
	targetHeight = (*env)->GetIntField(env, ths, FID_height);

	int i;
	const int filler = invert?0xFFFFFFFF:0xFF000000;
       	for(i = 0; i < targetWidth*targetHeight; i++) {
		targetBuf[i] = filler;
	}	

	textureWidth = makePow2(targetWidth);
	textureHeight = makePow2(targetHeight);

	windowWidth = w;
	windowHeight = h;

	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_RGBA,
		textureWidth,
		textureHeight,
		0,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		NULL);

	int uStop = 0x10000*targetWidth/textureWidth;
	int vStop = 0x10000*targetHeight/textureHeight;

	tex_coords[0] = 0;
	tex_coords[1] = 0;
	tex_coords[2] = 0;
	tex_coords[3] = vStop;
	tex_coords[4] = uStop;
	tex_coords[5] = vStop;
	tex_coords[6] = uStop;
	tex_coords[7] = 0;

	int xStop;
	int yStop;
	if(h > w) {
		xStop = 0x10000*w/h;
		yStop = 0x10000;
	} else {
		xStop = 0x10000;
		yStop = 0x10000*h/w;
	}
	vert_coords[0] = -xStop;
	vert_coords[1] = yStop;
	vert_coords[2] = -xStop;
	vert_coords[3] = -yStop;
	vert_coords[4] = xStop;
	vert_coords[5] = -yStop;
	vert_coords[6] = xStop;
	vert_coords[7] = yStop;

	glViewport(0,0, windowWidth, windowHeight);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	if(h > w) {
		glScalef((float)h/w,1,1);
	} else {
		glScalef(1,(float)w/h,1);
	}

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	if(invert) {
		glClearColorx(0x10000,0x10000,0x10000,0x10000);
	} else {
		glClearColorx(0,0,0,0x10000);
	}
	
	if(supersine) {
		for(i = 0; i < TABLE_SIZE; i++) {
			float x = 2*M_PI*i/TABLE_SIZE;
			float c = 0.80;
			sin_table[i] = 0x10000*sin(pow(sin(x)+c,3));
		}
	} else {
		for(i = 0; i < TABLE_SIZE; i++) {
			float x = 2*M_PI*i/TABLE_SIZE;
			sin_table[i] = 0x10000*sin(x);
		}
	}
}

void Java_as_adamsmith_iteration_SketchRenderer_nativeRender(JNIEnv* env, jobject ths, jlong progress) {

	const jobject jtargetBuf = (*env)->GetObjectField(env, ths, FID_targetBuf);
	int* targetBuf = (*env)->GetDirectBufferAddress(env, jtargetBuf);

	
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0,0, targetWidth, targetHeight, GL_RGBA, GL_UNSIGNED_BYTE, targetBuf);

	glClear(GL_COLOR_BUFFER_BIT);

	int fancyTrans = progress > 0;

	if(fancyTrans) {
		float k = pow(0.65,progress/1000.0);
		int angle = 45*0x10000*k;
		int boost = 0x10000 + 0x10000*k;
		glPushMatrix();
		glRotatex(angle,0,0,0x10000);
		glScalex(boost,boost,boost);
	}
	
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	if(fancyTrans) {	
		glPopMatrix();
	}

	GLenum err = glGetError();
	if(err != GL_NO_ERROR) {
		__android_log_print(ANDROID_LOG_INFO, LOG_TAG, "gl error: %d", err);
	}
	
}


inline int isin(int x) {
	return sin_table[(x>>(18-TABLE_BITS))&(TABLE_SIZE-1)];
}

inline int icos(int x) {
	return isin(x+0x3FFF);
}

void Java_as_adamsmith_iteration_SketchRenderer_nativeIterate(
		JNIEnv* env,
		jobject ths,
		jint t) {

	unsigned int i, j;

	const int niters = (*env)->GetIntField(env, ths, FID_niters);
	const int ncps = (*env)->GetIntField(env, ths, FID_ncps);

	const int width = (*env)->GetIntField(env, ths, FID_width);
	const int height = (*env)->GetIntField(env, ths, FID_height);

	const int xc = (*env)->GetIntField(env, ths, FID_xc);
	const int yc = (*env)->GetIntField(env, ths, FID_yc);
	const int xs = (*env)->GetIntField(env, ths, FID_xs);
	const int ys = (*env)->GetIntField(env, ths, FID_ys);

	const int ha = (*env)->GetIntField(env, ths, FID_a) >> 12;
	const int hb = (*env)->GetIntField(env, ths, FID_b) >> 12;
	const int hc = (*env)->GetIntField(env, ths, FID_c) >> 12;
	const int hd = (*env)->GetIntField(env, ths, FID_d) >> 12;

	const int vt = (*env)->GetIntField(env, ths, FID_vt);

	const int invert = (*env)->GetBooleanField(env, ths, FID_invert);

	const jobject jcolorLut = (*env)->GetObjectField(env, ths, FID_colorLut);
	const jobject jfancyBuf = (*env)->GetObjectField(env, ths, FID_fancyBuf);
	const jobject jtargetBuf = (*env)->GetObjectField(env, ths, FID_targetBuf);

	const int* colorLut = (*env)->GetDirectBufferAddress(env, jcolorLut);
	int* fancyBuf = (*env)->GetDirectBufferAddress(env, jfancyBuf);
	int* targetBuf = (*env)->GetDirectBufferAddress(env, jtargetBuf);

	for(i = 0; i < ncps; i++) {

		const int tt = vt*(t + rand()/(RAND_MAX/0x3FF));
		const int pp = isin(tt);
		const int qq = icos(tt);

		int x = rand()/(RAND_MAX/0x1FFFE)-0xFFFF;
		int y = rand()/(RAND_MAX/0x1FFFE)-0xFFFF;

		for(j = 0; j < niters; j++) {
			int xp, yp;
			if(random()&1) {
				xp = icos(ha*(x>>4) + hb*(y>>4)) + pp;
				yp = icos(hc*(x>>4) + hd*(y>>4)) + qq;
				x = xp + (yp>>2);
				y = yp - (xp>>2);
			} else {
				xp = icos(ha*(x>>4) + hb*(y>>4)) - qq;
				yp = isin(hd*(x>>4) + hc*(y>>4)) + pp;
				x = xp - (yp>>1);
				y = yp + (xp>>1);
			}

			const short scx = (short)((xc + ((xs>>12)*(x>>4))) >> 16);
			const short scy = (short)((yc + ((ys>>12)*(y>>4))) >> 16);
			
			if(scx < 0 || scx >= width) continue;
			if(scy < 0 || scy >= height) continue;

			const unsigned int pix = scy*width+scx;
			const unsigned int idx = pix*3;
		
			
			fancyBuf[idx+0] = MIN(fancyBuf[idx+0] + colorLut[3*j+0], 0xFFFF);
			fancyBuf[idx+1] = MIN(fancyBuf[idx+1] + colorLut[3*j+1], 0xFFFF);
			fancyBuf[idx+2] = MIN(fancyBuf[idx+2] + colorLut[3*j+2], 0xFFFF);

			int r = fancyBuf[idx+0]>>8;
			int g = fancyBuf[idx+1]>>8;
			int b = fancyBuf[idx+2]>>8;
			if(invert) {
				r = 255 - r;
				g = 255 - g;
				b = 255 - b;
			}
			targetBuf[pix] = 0xFF000000 | (b<<16) | (g<<8) | r;
		}
	}
}

void Java_as_adamsmith_iteration_SketchRenderer_nativeOnLoad(
		JNIEnv* env,
		jclass cls) {

	FID_niters = (*env)->GetFieldID(env, cls, "niters", "I");
	FID_ncps = (*env)->GetFieldID(env, cls, "ncps", "I");

	FID_width = (*env)->GetFieldID(env, cls, "width", "I");
	FID_height = (*env)->GetFieldID(env, cls, "height", "I");
	
	FID_xc = (*env)->GetFieldID(env, cls, "xc", "I");
	FID_yc = (*env)->GetFieldID(env, cls, "yc", "I");
	FID_xs = (*env)->GetFieldID(env, cls, "xs", "I");
	FID_ys = (*env)->GetFieldID(env, cls, "ys", "I");

	FID_a = (*env)->GetFieldID(env, cls, "a", "I");
	FID_b = (*env)->GetFieldID(env, cls, "b", "I");
	FID_c = (*env)->GetFieldID(env, cls, "c", "I");
	FID_d = (*env)->GetFieldID(env, cls, "d", "I");

	FID_vt = (*env)->GetFieldID(env, cls, "vt", "I");
	
	FID_invert = (*env)->GetFieldID(env, cls, "invert", "Z");

	FID_colorLut  = (*env)->GetFieldID(env, cls, "colorLut", "Ljava/nio/IntBuffer;");
	FID_fancyBuf  = (*env)->GetFieldID(env, cls, "fancyBuf", "Ljava/nio/IntBuffer;");
	FID_targetBuf = (*env)->GetFieldID(env, cls, "targetBuf", "Ljava/nio/IntBuffer;");
}
