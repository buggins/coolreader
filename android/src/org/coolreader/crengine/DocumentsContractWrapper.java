package org.coolreader.crengine;

import android.annotation.TargetApi;
import android.content.ContentResolver;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.os.Build;
import android.provider.DocumentsContract;
import android.util.Log;

import java.io.File;

/**
 * Collections of wrapper functions for DocumentsContract.
 */
public class DocumentsContractWrapper {
	private static final String TAG = "dcw";

	@TargetApi(Build.VERSION_CODES.LOLLIPOP)
	private static String queryFileName(Context context, Uri uri) {
		String name = null;
		Cursor cursor = null;
		try {
			cursor = context.getContentResolver().query(uri, new String[] { DocumentsContract.Document.COLUMN_DISPLAY_NAME }, null, null, null);
			cursor.moveToFirst();
			name = cursor.getString(0);
		} catch (Exception ignored) {}
		try {
			if (null != cursor)
				cursor.close();
		} catch (Exception ignored) {}
		return name;
	}

	/**
	 * Gets a uri equal to the given file information.
	 *
	 * @param fi file information
	 * @param context context
	 * @param topUri top level uri with required permission, for example, SD Card uri, returned by Intent.ACTION_OPEN_DOCUMENT_TREE.
	 * @return uri that equals fi
	 */
	@TargetApi(Build.VERSION_CODES.LOLLIPOP)
	public static Uri getDocumentUri(FileInfo fi, Context context, Uri topUri) {
		Uri uri = null;
		String filePath;
		if (fi.isArchive && fi.arcname != null)
			filePath = fi.arcname;
		else
			filePath = fi.pathname;
		ContentResolver resolver = context.getContentResolver();
		if (null != filePath) {
			filePath = new File(filePath).getAbsolutePath();
			// get doc ID for top URI
			String docId = DocumentsContract.getTreeDocumentId(topUri);
			if (null != docId)
				uri = DocumentsContract.buildDocumentUriUsingTree(topUri, docId);
			if (null != uri) {
				String[] parts = filePath.split("/");
				for (int i = 3; i < parts.length; i++) {
					// make special uri to fetch children items
					Uri childrenUri = DocumentsContract.buildChildDocumentsUriUsingTree(uri, DocumentsContract.getDocumentId(uri));
					Cursor cursor = null;
					boolean found = false;
					try {
						// query children
						cursor = resolver.query(childrenUri, new String[] { DocumentsContract.Document.COLUMN_DOCUMENT_ID }, null, null, null);
						while (cursor.moveToNext()) {
							String childId = cursor.getString(0);
							Uri childUri = DocumentsContract.buildDocumentUriUsingTree(uri, childId);
							String childName = queryFileName(context, childUri);
							// test child item name
							if (parts[i].equals(childName)) {
								// if equals jump to find inner child
								found = true;
								uri = childUri;
								break;
							}
						}
					} catch (Exception e) {
						Log.e(TAG, "document tree query failed: ", e);
					}
					try {
						if (null != cursor)
							cursor.close();
					} catch (Exception ignored) {}
					if (!found) {
						uri = null;
						break;
					}
				}
			}
		}
		return uri;
	}

	/**
	 * Delete file (or empty folder) by uri.
	 *
	 * @param context context
	 * @param fileUri uri for file
	 * @return operation result
	 */
	@TargetApi(Build.VERSION_CODES.LOLLIPOP)
	public static boolean deleteFile(Context context, Uri fileUri) {
		boolean res = false;
		try {
			res = DocumentsContract.deleteDocument(context.getContentResolver(), fileUri);
		} catch (Exception e) {
			Log.e(TAG, "delete document failed: ", e);
		}
		return res;
	}

	@TargetApi(Build.VERSION_CODES.LOLLIPOP)
	public static boolean fileExists(Context context, Uri fileUri) {
		ContentResolver resolver = context.getContentResolver();
		Cursor cursor;
		boolean res = false;
		try {
			cursor = resolver.query(fileUri, new String[] { DocumentsContract.Document.COLUMN_DOCUMENT_ID }, null, null, null);
			// If the document ID is found, then the file exists
			res = cursor.getCount() > 0;
		} catch (Exception e) {
			Log.e(TAG, "Failed query: " + e);
			return false;
		}
		try {
			cursor.close();
		} catch (Exception ignored) {}
		return res;
	}

	/**
	 * Gets the uri for working with the DocumentsContract using the uri returned by Intent.ACTION_OPEN_DOCUMENT_TREE.
	 *
	 * @param treeUri uri returned by Intent.ACTION_OPEN_DOCUMENT_TREE
	 * @return uri if operation successful, null otherwise
	 */
	@TargetApi(Build.VERSION_CODES.LOLLIPOP)
	public static Uri buildDocumentUriUsingTree(Uri treeUri) {
		String docId = DocumentsContract.getTreeDocumentId(treeUri);
		Uri uri = null;
		if (docId != null)
			uri = DocumentsContract.buildDocumentUriUsingTree(treeUri, docId);
		return uri;
	}

	@TargetApi(Build.VERSION_CODES.LOLLIPOP)
	public static Uri createFile(Context context, Uri parentUri, String mimeType, String filename) {
		Uri uri = null;
		try {
			uri = DocumentsContract.createDocument(context.getContentResolver(), parentUri, mimeType, filename);
		} catch (Exception e) {
			Log.e(TAG, "Failed to create file: ", e);
		}
		return uri;
	}
}
