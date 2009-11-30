package as.adamsmith.iteration;



import java.io.OutputStream;
import java.nio.IntBuffer;

import android.app.*;
import android.content.*;
import android.graphics.Bitmap;
import android.net.*;
import android.opengl.*;
import android.os.*;
import android.preference.PreferenceManager;
import android.provider.MediaStore;
import android.util.Log;
import android.view.*;
import android.widget.*;

public class Main extends Activity {
	GLSurfaceView surfaceView;
	SketchRenderer sketchRenderer;
	private Toast usageToast;
	
	private enum MenuEntries { SAVE, PREFERENCES };
	
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN|WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON, WindowManager.LayoutParams.FLAG_FULLSCREEN|WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        
        surfaceView = new GLSurfaceView(this);
        sketchRenderer = new SketchRenderer(this);
        surfaceView.setRenderer(sketchRenderer);
        setContentView(surfaceView);
        
        WindowManager.LayoutParams lp = getWindow().getAttributes();
        lp.screenBrightness = 1.0f;
        
        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(this);
        long lastLaunchTime = prefs.getLong("lastLaunchTime", 0L);
        if(lastLaunchTime + 86400*1000L < System.currentTimeMillis()) {
	        usageToast = new Toast(this);
	    	usageToast.setView(View.inflate(this, R.layout.usage, null));
	    	usageToast.setDuration(Toast.LENGTH_LONG);
	    	usageToast.show();
        }
    }
    
    @Override
    public void onPause() {
    	surfaceView.onPause();
    	super.onPause();
    }
    
    @Override
    public void onResume() {
    	super.onResume();
    	surfaceView.onResume();
    }
    
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
    	MenuItem saveItem = menu.add(0,MenuEntries.SAVE.ordinal(),0, getResources().getString(R.string.save_button));
    	saveItem.setIcon(android.R.drawable.ic_menu_save);
    	
    	MenuItem prefsItem = menu.add(0,MenuEntries.PREFERENCES.ordinal(),0, getResources().getString(R.string.preferences));
    	prefsItem.setIcon(android.R.drawable.ic_menu_preferences);
    	
    	return true;
    }
    
    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
    	if(item.getItemId() == MenuEntries.SAVE.ordinal()) {    		
    		saveTarget();
    	} else if (item.getItemId() == MenuEntries.PREFERENCES.ordinal()) {    		
    		Intent prefsIntent = new Intent(this, RenderPrefs.class);
    		startActivity(prefsIntent);
    	}
    	return true;
    }
    
    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
    	if(event.getAction() == KeyEvent.ACTION_DOWN && (keyCode == KeyEvent.KEYCODE_DPAD_CENTER || keyCode == KeyEvent.KEYCODE_CAMERA)) {
    		sketchRenderer.shouldReconfigure = true;
    		return true;
    	} else {
    		return super.onKeyDown(keyCode, event);
    	}
    }
    
	private void saveTarget() {
		final ProgressDialog progress = new ProgressDialog(this);
		progress.setIndeterminate(true);
		progress.setMessage(getResources().getString(R.string.saving));
		progress.show();
		
		new Thread() {
			public void run() {
				surfaceView.setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);
				try {
					final int w = sketchRenderer.width;
					final int h = sketchRenderer.height;
					final int[] pixels = new int[w*h];
					final IntBuffer targetBuf = sketchRenderer.targetBuf;
					
					// swap r and g channels, silly opengl
					for(int i = 0; i < w*h; i++) {
						final int in = targetBuf.get(i);
						final int ab = in & 0xFF00FF00;
						final int rg = in & 0x00FF00FF;
						final int out = ab | ((rg<<16)|(rg>>16));
						pixels[i] = out;
					}
					
					ContentResolver cr = getContentResolver();
					Bitmap bitmap = Bitmap.createBitmap(pixels, w, h, Bitmap.Config.ARGB_8888);
					String res = MediaStore.Images.Media.insertImage(cr, bitmap, null, null);
					Uri uri = Uri.parse(res);
					ContentValues values = new ContentValues(2);
					values.put(MediaStore.Images.Media.DISPLAY_NAME, getResources().getString(R.string.app_name));
					values.put(MediaStore.Images.Media.DATE_TAKEN, (int)System.currentTimeMillis());
					cr.update(uri, values, null, null);
					OutputStream os = cr.openOutputStream(uri);
					bitmap.compress(Bitmap.CompressFormat.PNG, 100, os);
					os.close();
					Intent intent = new Intent(Intent.ACTION_VIEW);
					intent.setData(uri);
					startActivity(intent);
				} catch (Exception e) {
					Log.e("iteration", "Failed while mangling content providers", e);
					Main.this.surfaceView.post(new Runnable() {
						public void run() {
							Toast.makeText(Main.this, getResources().getString(R.string.save_failed), Toast.LENGTH_LONG).show();
						}
					});
				} finally {
					progress.dismiss();
					surfaceView.setRenderMode(GLSurfaceView.RENDERMODE_CONTINUOUSLY);
				}
			}
		}.start();
	}
}
