package app.zxtune.fs.aygor;

import android.net.Uri;
import android.support.annotation.Nullable;
import android.text.TextUtils;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/*
 * 1) aygor:/${FilePath} - direct access to files or folders starting from http://abrimaal.pro-e.pl/ayon/ dir
 */
public final class Path implements app.zxtune.fs.httpdir.Path {

  private static final String SCHEME = "aygor";

  private final List<String> elements;

  private Path(List<String> elements) {
    this.elements = elements;
  }

  @Override
  public Uri[] getRemoteUris() {
    return new Uri[] {
        new Uri.Builder()
            .scheme("http")
            .authority("abrimaal.pro-e.pl")
            .path("ayon/" + getLocalId())
            .build()
    };
  }

  @Override
  public String getLocalId() {
    return TextUtils.join("/", elements);
  }

  @Override
  @Nullable
  public Path getParent() {
    final int count = elements.size();
    switch (count) {
      case 0:
        return null;
      case 1:
        return new Path(Collections.EMPTY_LIST);
      default:
        return new Path(elements.subList(0, count - 1));
    }
  }

  @Override
  public Uri getUri() {
    return new Uri.Builder()
            .scheme(SCHEME)
            .path(getLocalId())
            .build();
  }

  @Override
  @Nullable
  public String getName() {
    final int count = elements.size();
    return count > 0
            ? elements.get(count - 1)
            : null;
  }

  @Override
  public Path getChild(String name) {
    final ArrayList<String> result = new ArrayList<>(elements.size() + 1);
    result.addAll(elements);
    result.add(name);
    return new Path(result);
  }

  @Override
  public final boolean isEmpty() {
    return elements.isEmpty();
  }

  @Override
  public final boolean isFile() {
    return isFileName(getName());
  }

  private static boolean isFileName(@Nullable String component) {
    return component != null && component.toLowerCase().endsWith(".ay");
  }

  @Nullable
  public static Path parse(Uri uri) {
    if (SCHEME.equals(uri.getScheme())) {
      return new Path(uri.getPathSegments());
    } else {
      return null;
    }
  }

  public static Path create() {
    return new Path(Collections.EMPTY_LIST);
  }
}
