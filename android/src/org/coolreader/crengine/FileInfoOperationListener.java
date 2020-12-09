package org.coolreader.crengine;

public interface FileInfoOperationListener {
	/**
	 * Notify about file or directory operation result.
	 * @param fileInfo the object on which the operation was performed.
	 * @param errorStatus result of the operation, 0 - if successful, otherwise - error code.
	 */
	void onStatus(FileInfo fileInfo, int errorStatus);
}
