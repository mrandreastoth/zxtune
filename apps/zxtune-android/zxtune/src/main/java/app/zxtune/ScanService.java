/**
 *
 * @file
 *
 * @brief Scanning service
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune;

import android.app.IntentService;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Handler;
import android.os.Parcelable;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.widget.Toast;
import app.zxtune.device.ui.Notifications;
import app.zxtune.playback.Iterator;
import app.zxtune.playback.IteratorFactory;
import app.zxtune.playback.PlayableItem;
import app.zxtune.playback.stubs.PlayableItemStub;
import app.zxtune.playlist.Item;
import app.zxtune.playlist.PlaylistQuery;

import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;

public class ScanService extends IntentService {

  private static final String TAG = ScanService.class.getName();

  public static final String ACTION_START = TAG + ".add";
  public static final String ACTION_CANCEL = TAG + ".cancel";
  public static final String EXTRA_PATHS = "paths";

  private final Handler handler;
  private final NotifyTask tracking;
  private final InsertItemsThread insertThread;
  private final AtomicInteger addedItems;
  private Exception error;
  
  /**
   * InsertThread is executed for onCreate..onDestroy interval 
   */

  public ScanService() {
    super(ScanService.class.getName());
    this.handler = new Handler();
    this.tracking = new NotifyTask();
    this.insertThread = new InsertItemsThread();
    this.addedItems = new AtomicInteger();
    setIntentRedelivery(false);
  }

  @Override
  public void onCreate() {
    super.onCreate();
    makeToast(R.string.scanning_started, Toast.LENGTH_SHORT);
    insertThread.start();
  }

  @Override
  public void onStart(Intent intent, int startId) {
    if (ACTION_CANCEL.equals(intent.getAction())) {
      insertThread.cancel();
      stopSelf();
    } else {
      super.onStart(intent, startId);
    }
  }

  @Override
  public void onDestroy() {
    super.onDestroy();
    insertThread.flush();
    if (error != null) {
      final String msg = getString(R.string.scanning_failed, error.getMessage());
      makeToast(msg, Toast.LENGTH_LONG);
    } else {
      makeToast(R.string.scanning_stopped, Toast.LENGTH_SHORT);
    }
  }
  
  private void makeToast(int textRes, int duration) {
    makeToast(getString(textRes), duration);
  }
  
  private void makeToast(final String text, final int duration) {
    handler.post(new Runnable() {
      @Override
      public void run() {
        Toast.makeText(ScanService.this, text, duration).show();
      }
    });
  }

  @Override
  protected void onHandleIntent(Intent intent) {
    if (ACTION_START.equals(intent.getAction())) {
      final Parcelable[] paths = intent.getParcelableArrayExtra(EXTRA_PATHS);
      scan(paths);
    }
  }

  private void scan(Parcelable[] paths) {
    final Uri[] uris = new Uri[paths.length];
    System.arraycopy(paths, 0, uris, 0, uris.length);

    scan(uris);
  }

  private void scan(Uri[] uris) {
    for (Uri uri : uris) {
      Log.d(TAG, "scan on %s", uri);
    }
    try {
      error = null;
      final Iterator iter = IteratorFactory.createIterator(this, uris);
      do {
        final PlayableItem item = iter.getItem();
        try {
          insertThread.enqueue(item);
        } catch (InterruptedException e) {
          Log.d(TAG, "scan interrupted");
          item.release();
          break;
        }
      } while (insertThread.isActive() && iter.next());
    } catch (Exception e) {
      error = e;
      Log.w(TAG, e, "Scan failed");
      insertThread.cancel();
    }
  }

  private class InsertItemsThread extends Thread {

    private static final int MAX_QUEUE_SIZE = 100;
    private final ArrayBlockingQueue<PlayableItem> queue;
    private final AtomicBoolean active;

    InsertItemsThread() {
      super(TAG + ".InsertItemsThread");
      this.queue = new ArrayBlockingQueue<>(MAX_QUEUE_SIZE);
      this.active = new AtomicBoolean();
    }

    final void enqueue(PlayableItem item) throws InterruptedException {
      if (isActive()) {
        queue.put(item);
      } else {
        item.release();
      }
    }
    
    @Override
    public void start() {
      active.set(true);
      super.start();
    }
    
    final boolean isActive() {
      return active.get();
    }
    
    final void cancel() {
      Log.d(getName(), "cancel()");
      active.set(false);
      interrupt();
    }
    
    final void flush() {
      try {
        Log.d(getName(), "waitForFinish()");
        enqueue(PlayableItemStub.instance());
        active.set(false);
        join();
      } catch (InterruptedException e) {
        Log.w(getName(), e, "Failed to wait for finish");
      }
    }
    
    @Override
    public void run() {
      try {
        Log.d(getName(), "started");
        if (transferSingleItem()) {
          //start tracking only after first arrive
          performTransfer();
        }
      } catch (InterruptedException e) {
        Log.d(getName(), "interrupted");
      } catch (Exception e) {
        Log.w(getName(), e, "run()");
      } finally {
        Log.d(getName(), "stopping");
        cleanup();
      }
    }
    
    private void performTransfer() throws Exception {
      tracking.start();
      try {
        transferItems();
      } finally {
        tracking.stop();
      }
    }

    private void transferItems() throws Exception {
      for (;;) {
        if (!transferSingleItem()) {
          break;
        }
      }
    }
    
    private boolean transferSingleItem() throws Exception {
      final PlayableItem item = queue.take();
      if (item == PlayableItemStub.instance()) {
        return false;
      }
      insertItem(item);
      return true;
    }

    private void insertItem(PlayableItem item) throws Exception {
      try {
        final Item listItem = new Item(item.getDataId(), item.getModule());
        getContentResolver().insert(PlaylistQuery.ALL, listItem.toContentValues());
        addedItems.incrementAndGet();
      } finally {
        item.release();
      }
    }

    private void cleanup() {
      while (!queue.isEmpty()) {
        queue.remove().release();
      }
    }
  }

  private class NotifyTask implements Runnable {

    private static final int NOTIFICATION_DELAY = 100;
    private static final int NOTIFICATION_PERIOD = 2000;

    private WakeLock wakeLock;
    private StatusNotification notification;
    
    final void start() {
      notification = new StatusNotification();
      handler.postDelayed(this, NOTIFICATION_DELAY);
      getWakelock().acquire();
    }

    final void stop() {
      getWakelock().release();
      handler.removeCallbacks(this);
      notifyResolver();
      notification.hide();
      notification = null;
    }

    @Override
    public void run() {
      Log.d(TAG, "Notify about changes");
      handler.postDelayed(this, NOTIFICATION_PERIOD);
      notifyResolver();
      notification.show();
    }
    
    private WakeLock getWakelock() {
      if (wakeLock == null) {
        final PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
        wakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "ScanService");
      }
      return wakeLock;
    }

    private void notifyResolver() {
      getContentResolver().notifyChange(PlaylistQuery.ALL, null);
    }

    private class StatusNotification {

      private final CharSequence titlePrefix;
      private Notifications.Controller delegate;

      StatusNotification() {
        this.titlePrefix = getText(R.string.scanning_title);
        this.delegate = Notifications.createForService(ScanService.this, R.drawable.ic_stat_notify_scan);
        final Intent cancelIntent = new Intent(ScanService.this, ScanService.class);
        cancelIntent.setAction(ACTION_CANCEL);
        delegate.getBuilder()
            .setContentIntent(
                PendingIntent.getService(ScanService.this, 0, cancelIntent,
                    PendingIntent.FLAG_UPDATE_CURRENT)).setOngoing(true).setProgress(0, 0, true)
            .setContentTitle(titlePrefix)
            .setContentText(getText(R.string.scanning_text));
      }

      final void show() {
        final StringBuilder str = new StringBuilder();
        str.append(titlePrefix);
        str.append(" ");
        final int items = addedItems.get();
        str.append(getResources().getQuantityString(R.plurals.tracks, items, items));
        delegate.getBuilder().setContentTitle(str.toString());
        delegate.show();
      }

      final void hide() {
        delegate.hide();
      }
    }
  }
}
