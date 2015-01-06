package com.fsti.ladder.common;

import android.content.Context;
import android.media.MediaPlayer;

public class AlertSound {
	private MediaPlayer alertPlayer = null;
	private Context context;
	private boolean isPlaying = false;
	public AlertSound(Context context) {
		this.context = context;
	};
	
	public boolean isHaveResource()
	{
		if(alertPlayer == null)
		{
			return false;
		}
		return true;
	}
	
	public void setResource(int id)
	{
		if(alertPlayer != null)
		{
			alertPlayer.release();
		}
		alertPlayer = MediaPlayer.create(context, id);
	}
	
	public void setLooping(boolean loop)
	{
		alertPlayer.setLooping(loop);
	}

	public void starSound() {
		alertPlayer.start();
	}

	public boolean isPlaying() {
		return alertPlayer.isPlaying();
	}

	public void stopSound() {
		alertPlayer.stop();
	}

	public void pause() {
		alertPlayer.pause();
	}

	public void releaseSound() {
		if(alertPlayer != null)
		{
			alertPlayer.release();
		}
	}
}
