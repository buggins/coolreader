package org.coolreader.crengine;

import org.coolreader.db.CRDBService;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

import static org.coolreader.db.CRDBService.FileInfoLoadingCallback;

/**
 * Manages all content of the 'Browse file system' shelf
 */
public class FileSystemFolders extends FileInfoChangeSource {
    private static final FileInfoLoadingCallback NOOP = new FileInfoLoadingCallback(){
        @Override
        public void onFileInfoListLoaded(ArrayList<FileInfo> list) {

        }
    };
    private Scanner mScanner;

    private ArrayList<FileInfo> favoriteFolders = null;

    public FileSystemFolders(Scanner scanner) {
        this.mScanner = scanner;
    }

    private ArrayList<FileInfo> updateEntries(List<FileInfo> favoriteFolders){
        ArrayList<FileInfo> dirs = new ArrayList<FileInfo>();
        File[] roots = Engine.getStorageDirectories(false);
        for (File f : roots) {
            FileInfo dir = new FileInfo(f);
            dir.setType(FileInfo.TYPE_FS_ROOT);
            dirs.add(dir);
        }
        dirs.addAll(filter(favoriteFolders));
        if (Services.getScanner() != null) {
            FileInfo downloadDirectory = mScanner.getDownloadDirectory();
            downloadDirectory.setType(FileInfo.TYPE_DOWNLOAD_DIR);
            dirs.add(downloadDirectory);
        }
        return dirs;
    }

    private ArrayList<FileInfo> filter(List<FileInfo> favoriteFolders) {
        ArrayList<FileInfo> filtered = new ArrayList<FileInfo>();
        for(FileInfo fi: favoriteFolders){
            if(mScanner.isValidFolder(fi))
                filtered.add(fi);
        }
        return filtered;
    }

    public void loadFavoriteFolders(final CRDBService.LocalBinder binder) {
        loadFavoriteFoldersAndDo(binder, NOOP);
    }

    public ArrayList<FileInfo> getFileSystemFolders() {
        return updateEntries(favoriteFolders);
    }


    public boolean canMove(FileInfo folder, boolean left) {
        int folderIndex = findFavoriteFolder(folder);
        if(folderIndex == -1)
            return false;
        int increment = left? -1: 1;
        int newIndex = folderIndex + increment;
        return newIndex >= 0 && newIndex < favoriteFolders.size();
    }

    public void addFavoriteFolder(final CRDBService.LocalBinder binder, final FileInfo folder){
        loadFavoriteFoldersAndDo(binder, new FileInfoLoadingCallback(){
            @Override
            public void onFileInfoListLoaded(ArrayList<FileInfo> list) {
                if(findFavoriteFolder(folder) != -1)
                    return;
                FileInfo dbFolder = new FileInfo(folder);
                int maxPos = 0;
                if(!favoriteFolders.isEmpty()){
                    FileInfo lastFolder = favoriteFolders.get(favoriteFolders.size()-1);
                    maxPos = lastFolder.seriesNumber;
                }
                dbFolder.seriesNumber = maxPos + 1;
                binder.createFavoriteFolder(dbFolder);
                favoriteFolders.add(dbFolder);
                onChange(dbFolder, false);
            }
        });
    }

    public void moveFavoriteFolder(final CRDBService.LocalBinder binder, final FileInfo folder, final boolean left){
        loadFavoriteFoldersAndDo(binder, new FileInfoLoadingCallback(){
            @Override
            public void onFileInfoListLoaded(ArrayList<FileInfo> list) {
                int folderIndex = findFavoriteFolder(folder);
                if(folderIndex == -1)
                    return;
                //find new place
                int increment = left? -1: 1;
                int newIndex = folderIndex + increment;
                if(newIndex < 0 || newIndex >= favoriteFolders.size()){
                    return;
                }
                //update this folder index and neighbor's one
                FileInfo neighbor = favoriteFolders.get(newIndex);
                int neighbourIdx = neighbor.seriesNumber;
                neighbor.seriesNumber = folder.seriesNumber;
                folder.seriesNumber = neighbourIdx;
                //update place
                favoriteFolders.set(folderIndex, neighbor);
                favoriteFolders.set(newIndex, folder);
                //save changes
                binder.updateFavoriteFolder(folder);
                binder.updateFavoriteFolder(neighbor);
                onChange(folder, false);
            }
        });
    }

    public void removeFavoriteFolder(final CRDBService.LocalBinder binder, final FileInfo folder){
        loadFavoriteFoldersAndDo(binder, new FileInfoLoadingCallback(){
            @Override
            public void onFileInfoListLoaded(ArrayList<FileInfo> list) {
                int folderIndex = findFavoriteFolder(folder);
                if(folderIndex == -1)
                    return;
                binder.deleteFavoriteFolder(folder);
                favoriteFolders.remove(folderIndex);
                onChange(null, false);
            }
        });
    }


    private void loadFavoriteFoldersAndDo(final CRDBService.LocalBinder binder, final FileInfoLoadingCallback callback) {
        if(favoriteFolders == null) {
            binder.loadFavoriteFolders(new FileInfoLoadingCallback() {
                @Override
                public void onFileInfoListLoaded(ArrayList<FileInfo> list) {
                    favoriteFolders = new ArrayList<FileInfo>(list);
                    callback.onFileInfoListLoaded(favoriteFolders);
                    onChange(null,false);
                }
            });
        } else {
            callback.onFileInfoListLoaded(favoriteFolders);
        }

    }

    private int findFavoriteFolder(FileInfo folder){
        if(folder == null || favoriteFolders == null)
            return -1;
        int size = favoriteFolders.size();
        for(int idx = 0; idx < size; ++idx){
            if(favoriteFolders.get(idx).pathNameEquals(folder)){
                return idx;
            }
        }
        return -1;
    }
}
