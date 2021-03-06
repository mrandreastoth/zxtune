/**
 *
 * @file
 *
 * @brief Stub singleton implementation of PlayerEventsListener
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.sound;

import android.support.annotation.NonNull;

public class StubPlayerEventsListener implements PlayerEventsListener {

  // permit inheritance
  protected StubPlayerEventsListener() {}
  
  @Override
  public void onStart() {}

  @Override
  public void onFinish() {}

  @Override
  public void onStop() {}

  @Override
  public void onPause() {}
  
  @Override
  public void onError(@NonNull Exception e) {}

  public static PlayerEventsListener instance() {
    return Holder.INSTANCE;
  }

  //onDemand holder idiom
  private static class Holder {
    public static final PlayerEventsListener INSTANCE = new StubPlayerEventsListener();
  }
}
