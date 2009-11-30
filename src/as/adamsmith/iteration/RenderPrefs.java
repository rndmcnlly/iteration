package as.adamsmith.iteration;

import android.os.Bundle;
import android.preference.PreferenceActivity;

public class RenderPrefs extends PreferenceActivity {
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		addPreferencesFromResource(R.xml.prefs);
	}
}