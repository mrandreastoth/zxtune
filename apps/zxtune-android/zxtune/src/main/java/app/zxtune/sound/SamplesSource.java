/**
 *
 * @file
 *
 * @brief Samples source interface
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.sound;

import android.support.annotation.NonNull;

public interface SamplesSource {

  final class Channels {
    public static final int COUNT = 2;
  }

  final class Sample {
    public static final int BYTES = 2;
  }
  
  /**
   * Initialize stream for input
   * @param sampleRate required sample rate in Hz (e.g. 44100)
   */
  void initialize(int sampleRate) throws Exception;
  
  /**
   * Acquire next sound chunk
   * @param buf result buffer of 16-bit signed interleaved stereo signal
   * @return true if buffer filled
   */
  boolean getSamples(@NonNull short[] buf) throws Exception;

  /**
   * Reset playback to initial position keeping sampleRate and other params intact
   * @throws Exception
   */
  void reset() throws Exception;
  
  /**
   * Release all internal resources
   */
  void release();
}
