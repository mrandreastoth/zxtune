/**
 * @file
 * @brief
 * @version $Id:$
 * @author
 */
package app.zxtune.fs.zxtunes;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;

import android.content.Context;
import android.util.Log;

/**
 * 
 */
final class CachingCatalog extends Catalog {
  
  private final static String TAG = CachingCatalog.class.getName();
  
  private final static String CACHE_DIR_NAME = "www.zxtunes.com";

  private final String cacheDir;
  private final Catalog remote;
  private final Database db;
  
  /**
   * 
   */
  public CachingCatalog(Context context, Catalog remote, Database db) {
    this.cacheDir = context.getCacheDir().getAbsolutePath() + File.separator + CACHE_DIR_NAME + File.separator;
    this.remote = remote;
    this.db = db;
    createCacheDir();
  }
  
  private void createCacheDir() {
    final File dir = new File(cacheDir);
    if (!dir.exists()) {
      dir.mkdirs();
    }
  }

  @Override
  public void queryAuthors(AuthorsVisitor visitor, Integer id) {
    final CountingAuthorsVisitor count = new CountingAuthorsVisitor(visitor);
    db.queryAuthors(count, id);
    if (0 == count.get()) {
      Log.d(TAG, "Authors cache is empty. Query from remote");
      final Database.Transaction transaction = db.startTransaction();
      try {
        remote.queryAuthors(new CachingAuthorsVisitor(count), id);
        transaction.succeed();
        Log.d(TAG, "Cached " + count.get() + " authors");
      } finally {
        transaction.finish();
      }
    }
  }
  
  @Override
  public void queryTracks(TracksVisitor visitor, Integer id, Integer author) {
    //TODO: make another way to check if cache is filled
    final CountingTracksVisitor count = new CountingTracksVisitor(visitor);
    db.queryTracks(count, id, author);
    if (0 == count.get()) {
      Log.d(TAG, "Tracks cache is empty for id=" + id + " author=" + author);
      final Database.Transaction transaction = db.startTransaction();
      try {
        remote.queryTracks(new CachingTracksVisitor(count, author), id, author);
        transaction.succeed();
        Log.d(TAG, "Cached " + count.get() + " tracks");
      } finally {
        transaction.finish();
      }
    }
  }
  
  @Override
  public byte[] getTrackContent(int id) throws IOException {
    final File cache = new File(cacheDir + Integer.toString(id));
    try {
      if (cache.exists()) {
        Log.d(TAG, "Reading content of track " + id + " from cache");
        return readFrom(cache);
      }
    } catch (IOException e) {
      Log.d(TAG, "Failed to read from cache", e);
    }
    final byte[] content = remote.getTrackContent(id);
    Log.d(TAG, "Write content of track " + id + " to cache");
    writeTo(cache, content);
    return content;
  }
  
  private byte[] readFrom(File file) throws IOException {
    final FileInputStream stream = new FileInputStream(file);
    try {
      final int len = (int) file.length();
      final byte[] content = new byte[len];
      stream.read(content, 0, len);
      return content;
    } finally {
      stream.close();
    }
  }
  
  private void writeTo(File file, byte[] data) {
    try {
      final FileOutputStream stream = new FileOutputStream(file);
      try {
        stream.write(data);
      } finally {
        stream.close();
      }
    } catch (IOException e) {
      Log.d(TAG, "writeTo " + file.getAbsolutePath(), e);
      file.delete();
    }
  }
  
  private static class CountingAuthorsVisitor implements AuthorsVisitor {

    private AuthorsVisitor delegate;
    private int count;
    
    CountingAuthorsVisitor(AuthorsVisitor delegate) {
      this.delegate = delegate;
      this.count = 0;
    }
    
    @Override
    public void accept(Author obj) {
      delegate.accept(obj);
      ++count;
    }
    
    final int get() {
      return count;
    }
  }
  
  private class CachingAuthorsVisitor implements AuthorsVisitor {
    
    private AuthorsVisitor delegate;
    
    CachingAuthorsVisitor(AuthorsVisitor delegate) {
      this.delegate = delegate;
    }

    @Override
    public void accept(Author obj) {
      delegate.accept(obj);
      try {
        db.addAuthor(obj);
      } catch (Exception e) {
        Log.d(TAG, "acceptAuthor()", e);
      }
    }
  }
  
  private static class CountingTracksVisitor implements TracksVisitor {
    
    private final TracksVisitor delegate;
    private int count;
    
    CountingTracksVisitor(TracksVisitor delegate) {
      this.delegate = delegate;
      this.count = 0;
    }

    @Override
    public void accept(Track obj) {
      delegate.accept(obj);
      ++count;
    }
    
    final int get() {
      return count;
    }
  }
  
  private class CachingTracksVisitor implements TracksVisitor {
    
    private final TracksVisitor delegate;
    private final Integer author;
    
    CachingTracksVisitor(TracksVisitor delegate, Integer author) {
      this.delegate = delegate;
      this.author = author;
    }

    @Override
    public void accept(Track obj) {
      delegate.accept(obj);
      try {
        db.addTrack(obj, author);
      } catch (Exception e) {
        Log.d(TAG, "addTrack()", e);
      }
    }
  }
}
