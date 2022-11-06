/*
 * CoolReader for Android
 * Copyright (C) 2012 Vadim Lopatin <coolreader.org@gmail.com>
 * Copyright (C) 2021 Aleksey Chernov <valexlin@gmail.com>
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

package org.coolreader;

import android.annotation.SuppressLint;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.telephony.PhoneStateListener;
import android.telephony.TelephonyManager;

import org.coolreader.crengine.L;
import org.coolreader.crengine.Logger;

public class PhoneStateReceiver extends BroadcastReceiver {
	public static final Logger log = L.create("phsr");

	private static Runnable onPhoneActivityStartedHandler;
	public static boolean listenerIsRegistered;

	public static class CustomPhoneStateListener extends PhoneStateListener {

		int lastState = -1;

		@Override
		public void onCallStateChanged(int state, String incomingNumber) {
			log.v("onCallStateChange state=" + state);
			if (state == lastState)
				return;
			lastState = state;
			switch (state) {
				case TelephonyManager.CALL_STATE_IDLE:
//					log.d("call state: IDLE");
//					if (onPhoneActivityStartedHandler != null)
//					onPhoneActivityStartedHandler.run();
					break;
				case TelephonyManager.CALL_STATE_RINGING:
					log.d("call state: RINGING");
					if (onPhoneActivityStartedHandler != null)
						onPhoneActivityStartedHandler.run();
					break;
				case TelephonyManager.CALL_STATE_OFFHOOK:
					log.d("call state: OFFHOOK");
					if (onPhoneActivityStartedHandler != null)
						onPhoneActivityStartedHandler.run();
					break;
			}
		}
	}

	@SuppressLint("UnsafeProtectedBroadcastReceiver")
	@Override
	public void onReceive(Context context, Intent intent) {
		if (!listenerIsRegistered) {
			try {
				TelephonyManager telephony = (TelephonyManager) context.getSystemService(Context.TELEPHONY_SERVICE);
				CustomPhoneStateListener customPhoneListener = new CustomPhoneStateListener();
				telephony.listen(customPhoneListener, PhoneStateListener.LISTEN_CALL_STATE);
				listenerIsRegistered = true;
				log.v("phone state listener is registered.");
			} catch (Exception e) {
				log.e("Failed to register phone state listener: " + e.toString());
			}
		}
	}

	public static void setPhoneActivityHandler(Runnable handler) {
		onPhoneActivityStartedHandler = handler;
	}

	protected void finalize() throws Throwable {
		super.finalize();
		if (listenerIsRegistered) {
			listenerIsRegistered = false;
			log.v("phone state listener is DESTROYED.");
		}
	}
}
