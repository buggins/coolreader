/*
 *   Copyright (C) 2020 by Chernov A.A.
 *   valexlin@gmail.com
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

package org.coolreader.sync2;

public interface OnOperationCompleteListener<TResult> {
	/**
	 * Called after the task completes, regardless of the result.
	 *
	 * @param result Result of task, may be null.
	 * @param ok mark of the successful completion of the operation, i.e. without any I/O errors.
	 */
	void onCompleted(TResult result, boolean ok);

	/**
	 * Called when the task has failed.
	 *
	 * @param e
	 */
	void onFailed(Exception e);
}
