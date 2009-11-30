package as.adamsmith.iteration;

import java.nio.*;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import android.content.SharedPreferences;
import android.opengl.GLSurfaceView;
import android.preference.PreferenceManager;
import android.util.Log;


public class SketchRenderer implements GLSurfaceView.Renderer {
	Main main;
	boolean shouldReconfigure;
	
	boolean boost;
	boolean invert;
	boolean supersine;
	boolean reconfigureSelf;
	
	int frameNo;
	long lastFrameTick;
	long introTick;
	
	final int niters = 10;
	final int ncps = 1024;
	
	final int vt = 19;
	
	int width, height;
	int xc, xs, yc, ys; // Q16
	int a, b, c, d;  // Q16
	
	IntBuffer colorLut;  // Q8, one int per channel
	IntBuffer fancyBuf; // Q8, one int per channel
	IntBuffer targetBuf; // ARGB8, one int per pixels
	
	static {
		System.loadLibrary("iteration");
		nativeOnLoad();
	}
	
	private static native void nativeOnLoad();
	
	private native void nativeIterate(long t);
	
	private native void nativeInit();
	private native void nativeResize(int w, int h, boolean supersine);
	private native void nativeRender(long progress);

	public SketchRenderer(Main m) {
		main = m;
	}
	
	public void reconfigure(int w, int h) {
		SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(main);
		
		String scaleFactorString = prefs.getString("scaleFactor", "1.0");
		float scaleFactor = Float.parseFloat(scaleFactorString);
		
		boost = prefs.getBoolean("introAnimations", true);
		invert = prefs.getBoolean("invertColors", false);
		supersine = prefs.getBoolean("supersine", false);
		reconfigureSelf = prefs.getBoolean("reconfigureSelf", false);
		
		introTick = System.currentTimeMillis();
		
		width = (int)(w*scaleFactor);
		height = (int)(h*scaleFactor);
		Log.i("iteration", "target is "+width+"x"+height+" from scale factor "+scaleFactor);
		
		int dia = Math.min(width, height)*(prefs.getBoolean("focusCore", false)?2:1);
		xc = 0xFFFF*width/2;
		xs = (int)(0x10000*0.6f*dia/(float)Math.PI);
		yc = 0xFFFF*height/2;
		ys = (int)(0x10000*0.6f*dia/(float)Math.PI);
		
		float k = 1.f;
		a = (int)(0x10000 * k * Math.random()*Math.random())*(Math.random()<0.5?-1:1);
		b = (int)(0x10000 * k * Math.random()*Math.random())*(Math.random()<0.5?-1:1);
		c = (int)(0x10000 * k * Math.random()*Math.random())*(Math.random()<0.5?-1:1);
		d = (int)(0x10000 * k * Math.random()*Math.random())*(Math.random()<0.5?-1:1);
		
		Log.i("iteration", "a=" + String.format("%1.3f", (float)a/0x10000));
		Log.i("iteration", "b=" + String.format("%1.3f", (float)b/0x10000));
		Log.i("iteration", "c=" + String.format("%1.3f", (float)c/0x10000));
		Log.i("iteration", "d=" + String.format("%1.3f", (float)d/0x10000));
		
		colorLut = ByteBuffer
			.allocateDirect(niters*3*4)
			.order(ByteOrder.nativeOrder())
			.asIntBuffer();
		
		String pointQualityString = prefs.getString("pointQuality", "1.0");
		float pointQuality = Float.parseFloat(pointQualityString);
		
		float alpha = 320f/(pointQuality*pointQuality);
		
		List<Float> colorFoo = new ArrayList<Float>(3);
		colorFoo.add(1f);
		colorFoo.add((float)Math.random());
		colorFoo.add(0f);
		Collections.shuffle(colorFoo);
		
		for(int iter=0; iter < niters; iter++) {
			float t = (iter+0.5f)/niters;
			colorLut.put(3*iter+0, 1+(int)(alpha*(t*colorFoo.get(0)+(1-t)*colorFoo.get(1))));
			colorLut.put(3*iter+1, 1+(int)(alpha*(t*colorFoo.get(1)+(1-t)*colorFoo.get(2))));
			colorLut.put(3*iter+2, 1+(int)(alpha*(t*colorFoo.get(2)+(1-t)*colorFoo.get(0))));
		}
		
		fancyBuf = ByteBuffer
		.allocateDirect(width*height*3*4)
		.order(ByteOrder.nativeOrder())
		.asIntBuffer();
		
		targetBuf = ByteBuffer
		.allocateDirect(width*height*4)
		.order(ByteOrder.nativeOrder())
		.asIntBuffer();
		
		
		System.gc();
	}
	
	@Override
	public void onDrawFrame(GL10 gl) {
		if(reconfigureSelf) {
			long now = System.currentTimeMillis();
			if(now > introTick + 15*1000l) {
				shouldReconfigure = true;
			}
		}
		
		if(shouldReconfigure) {
			onSurfaceChanged(gl, main.surfaceView.getWidth(),main.surfaceView.getHeight());
			shouldReconfigure = false;
		}
		
		if(frameNo++ % 60 == 0) {
			long now = System.currentTimeMillis();
			float fps = 1000.0f/(now-lastFrameTick);
			Log.i("iteration", "fps="+fps);
		}
		
		lastFrameTick = System.currentTimeMillis();
		nativeIterate(lastFrameTick);
		
		if(boost) {
			nativeRender(lastFrameTick - introTick);
		} else {
			nativeRender(0L);
		}
	}
	@Override
	public void onSurfaceChanged(GL10 gl, int width, int height) {
		reconfigure(width, height);
		nativeResize(width, height, supersine);
	}
	@Override
	public void onSurfaceCreated(GL10 gl, EGLConfig config) {
		nativeInit();
	}
	
}