/*
 * CoolReader for Android
 * Copyright (C) 2012 Vadim Lopatin <coolreader.org@gmail.com>
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

package org.coolreader.plugins;

import java.io.File;

import org.coolreader.plugins.litres.ResponseCallback;

import android.util.Log;

public class FileResponse implements ResponseCallback, AsyncResponse {
	private int errorCode = -1;
	private String errorMessage;
	public OnlineStoreBook book;
	public File fileToSave;
	public boolean isTrial;

	public FileResponse(OnlineStoreBook book, File fileToSave, boolean isTrial) {
		this.book = book;
		this.fileToSave = fileToSave;
		this.isTrial = isTrial;
	}
	
	@Override
	public void onError(int errorCode, String errorMessage) {
		Log.e("litres", "error " + errorCode + ": " + errorMessage);
		this.errorCode = errorCode;
		this.errorMessage = errorMessage;
	}
	@Override
	public AsyncResponse getResponse() {
		if (errorCode != -1)
			return new ErrorResponse(errorCode, errorMessage);
		return this;
	}
}
