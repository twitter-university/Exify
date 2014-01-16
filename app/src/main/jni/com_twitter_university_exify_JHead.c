#include <jni.h>
#include <sys/stat.h>
#include <android/log.h>
#include "jhead.h"

#define LOG_TAG "JHead.c"
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))
#ifdef DEBUG
#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#else
#define LOGD(...) ((void)0)
#endif

static jclass mapClass;
static jmethodID mapConstructorMethod;
static jmethodID mapPutMethod;

static int initGlobals(JNIEnv *env) {
  LOGD("Initializing globals");
  mapClass = (*env)->FindClass(env, "java/util/TreeMap");
  if (mapClass) {
    mapConstructorMethod = (*env)->GetMethodID(env, mapClass, "<init>", "()V");
    mapPutMethod = (*env)->GetMethodID(env, mapClass, "put",
        "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
    if (mapConstructorMethod && mapPutMethod) {
      mapClass = (*env)->NewGlobalRef(env, mapClass);
      return 0;
    }
  }
  LOGE("Failed to init");
  return -1;
}

static void mapPut(JNIEnv *env, jobject map, const char* key, const char* value) {
  if (value && value[0]) {
    jstring jkey = (*env)->NewStringUTF(env, key);
    jstring jvalue = (*env)->NewStringUTF(env, value);
    if (jkey && jvalue) {
      LOGD("Added (%s, %s) to map", key, value);
      (*env)->CallObjectMethod(env, map, mapPutMethod, jkey, jvalue);
    } else {
      LOGE("Failed to map.put(%s, %s)", key, value);
    }
  } else {
    LOGD("Ignoring map.put(%s, <empty>)", key);
  }
}

static int loadImageInfo(JNIEnv *env, const char* filename) {
  struct stat st;
  LOGD("Loading image info for %s", filename);
  ResetJpgfile();
  memset(&ImageInfo, 0, sizeof(ImageInfo));
  ImageInfo.FlashUsed = -1;
  ImageInfo.MeteringMode = -1;
  ImageInfo.Whitebalance = -1;
  if (stat(filename, &st) >= 0) {
    ImageInfo.FileDateTime = st.st_mtime;
    ImageInfo.FileSize = st.st_size;
  }
  strncpy(ImageInfo.FileName, filename, PATH_MAX);
  if (ReadJpegFile(filename, READ_METADATA)) {
    return TRUE;
  } else {
    jclass ioExceptionClass = (*env)->FindClass(env, "java/io/IOException");
    char message[128];
    snprintf(message, sizeof(message), "Failed to read jpeg file %s", filename);
    (*env)->ThrowNew(env, ioExceptionClass, message);
    LOGE("Failed to read jpeg file %s", filename);
    return FALSE;
  }
}

static jbyteArray getThumbnail(JNIEnv *env, jclass clazz, jstring jfilename) {
  Section_t* exifSection = NULL;
  jbyteArray byteArray = NULL;
  const char* filename = (*env)->GetStringUTFChars(env, jfilename, NULL);
  if (!filename) {
    return NULL;
  }
  if (!loadImageInfo(env, filename)) {
    LOGE("Failed to get thumbnail for %s", filename);
    goto done;
  }
  LOGD("Getting thumbnail for %s", filename);
  exifSection = FindSection(M_EXIF);
  if (!exifSection) {
    LOGD("No EXIF data in %s", filename);
    goto done;
  }
  if (ImageInfo.ThumbnailSize == 0) {
    LOGD("No Thumbnail in %s", filename);
    goto done;
  }
  byteArray = (*env)->NewByteArray(env, ImageInfo.ThumbnailSize);
  if (!byteArray) {
    LOGE( "Failed to allocate Java array of %d bytes", ImageInfo.ThumbnailSize);
    goto done;
  }
  (*env)->SetByteArrayRegion(env, byteArray, 0, ImageInfo.ThumbnailSize,
      exifSection->Data + ImageInfo.ThumbnailOffset + 8);
  LOGD("Got thumbnail for %s of %d bytes", filename, ImageInfo.ThumbnailSize);
  done:
  DiscardData();
  (*env)->ReleaseStringUTFChars(env, jfilename, filename);
  return byteArray;
}

static jobject getImageInfo(JNIEnv *env, jclass clazz, jstring jfilename) {
  jobject map = (*env)->NewObject(env, mapClass, mapConstructorMethod);
  #define STRLEN 128
  char str[STRLEN];
  const char* filename = (*env)->GetStringUTFChars(env, jfilename, NULL);
  if (!filename) {
    return NULL;
  }
  if (!loadImageInfo(env, filename)) {
    LOGE("Failed to get image info for %s", filename);
    goto done;
  }
  LOGD("Getting image info for %s", filename);

  mapPut(env, map, "File Name", ImageInfo.FileName);

  snprintf(str, STRLEN, "%d", ImageInfo.FileSize);
  mapPut(env, map, "File Size", str);

  mapPut(env, map, "Camera Make", ImageInfo.CameraMake);

  mapPut(env, map, "Camera Model", ImageInfo.CameraModel);

  mapPut(env, map, "Date/Time", ImageInfo.DateTime);

  snprintf(str, STRLEN, "%d x %d", ImageInfo.Width, ImageInfo.Height);
  mapPut(env, map, "Resolution", str);

  snprintf(str, STRLEN, "%d", ImageInfo.Width);
  mapPut(env, map, "Width", str);

  snprintf(str, STRLEN, "%d", ImageInfo.Height);
  mapPut(env, map, "Height", str);

  char *orientation = NULL;
  switch (ImageInfo.Orientation) {
  case 0:
    orientation = "Undefined";
    break;
  case 1:
    orientation = "Normal";
    break;
  case 2:
    orientation = "Flip Horizontal";
    break;
  case 3:
    orientation = "Rotate 180";
    break;
  case 4:
    orientation = "Flip Vertical";
    break;
  case 5:
    orientation = "Transpose";
    break;
  case 6:
    orientation = "Rotate 90";
    break;
  case 7:
    orientation = "Transverse";
    break;
  case 8:
    orientation = "Rotate 270";
    break;
  }
  mapPut(env, map, "Orientation", orientation);

  if (ImageInfo.IsColor == 0) {
    mapPut(env, map, "Color/B&W", "Black and White");
  }

  if (ImageInfo.FlashUsed >= 0) {
    char *flashUsed = "unknown";
    if (ImageInfo.FlashUsed & 1) {
      flashUsed = "Yes";
      switch (ImageInfo.FlashUsed) {
      case 0x5:
        flashUsed = "Yes (Strobe light not detected)";
        break;
      case 0x7:
        flashUsed = "Yes (Strobe light detected) ";
        break;
      case 0x9:
        flashUsed = "Yes (manual)";
        break;
      case 0xd:
        flashUsed = "Yes (manual, return light not detected)";
        break;
      case 0xf:
        flashUsed = "Yes (manual, return light  detected)";
        break;
      case 0x19:
        flashUsed = "Yes (auto)";
        break;
      case 0x1d:
        flashUsed = "Yes (auto, return light not detected)";
        break;
      case 0x1f:
        flashUsed = "Yes (auto, return light detected)";
        break;
      case 0x41:
        flashUsed = "Yes (red eye reduction mode)";
        break;
      case 0x45:
        flashUsed = "Yes (red eye reduction mode return light not detected)";
        break;
      case 0x47:
        flashUsed = "Yes (red eye reduction mode return light  detected)";
        break;
      case 0x49:
        flashUsed = "Yes (manual, red eye reduction mode)";
        break;
      case 0x4d:
        flashUsed =
            "Yes (manual, red eye reduction mode, return light not detected)";
        break;
      case 0x4f:
        flashUsed = "Yes (red eye reduction mode, return light detected)";
        break;
      case 0x59:
        flashUsed = "Yes (auto, red eye reduction mode)";
        break;
      case 0x5d:
        flashUsed =
            "Yes (auto, red eye reduction mode, return light not detected)";
        break;
      case 0x5f:
        flashUsed = "Yes (auto, red eye reduction mode, return light detected)";
        break;
      }
    } else {
      flashUsed = "No";
      switch (ImageInfo.FlashUsed) {
      case 0x18:
        flashUsed = "No (auto)";
        break;
      }
    }
    mapPut(env, map, "Flash Used", flashUsed);
  }

  if (ImageInfo.FocalLength) {
    if (ImageInfo.FocalLength35mmEquiv) {
      snprintf(str, STRLEN, "%4.1fmm (35mm equivalent: %dmm)",
          ImageInfo.FocalLength, ImageInfo.FocalLength35mmEquiv);
    } else {
      snprintf(str, STRLEN, "%4.1fmm", ImageInfo.FocalLength);
    }
    mapPut(env, map, "Focal Length", str);
  }

  if (ImageInfo.DigitalZoomRatio > 1) {
    snprintf(str, STRLEN, "%1.3fx", (double) ImageInfo.DigitalZoomRatio);
    mapPut(env, map, "Digital Zoom", str);
  }

  if (ImageInfo.CCDWidth) {
    snprintf(str, STRLEN, "%4.2fmm", (double) ImageInfo.CCDWidth);
    mapPut(env, map, "CCD Width", str);
  }

  if (ImageInfo.ExposureTime) {
    snprintf(str, STRLEN,
        ImageInfo.ExposureTime < 0.010 ? "%6.4f s (1/%d)" :
        ImageInfo.ExposureTime <= 0.5 ? "%5.3f s (1/%d)" : "%5.3f s",
        (double) ImageInfo.ExposureTime,
        (int) (0.5 + 1 / ImageInfo.ExposureTime));
    mapPut(env, map, "Exposure Time", str);
  }

  if (ImageInfo.ApertureFNumber) {
    snprintf(str, STRLEN, "f/%3.1f", (double) ImageInfo.ApertureFNumber);
    mapPut(env, map, "Aperture", str);
  }

  if (ImageInfo.Distance) {
    if (ImageInfo.Distance < 0) {
      snprintf(str, STRLEN, "%s", "Infinite");
    } else {
      snprintf(str, STRLEN, "%4.2fm", (double) ImageInfo.Distance);
    }
    mapPut(env, map, "Focus Distance", str);
  }

  if (ImageInfo.ISOequivalent) {
    snprintf(str, STRLEN, "%2d", (int) ImageInfo.ISOequivalent);
    mapPut(env, map, "ISO Equivalent", str);
  }

  if (ImageInfo.ExposureBias) {
    snprintf(str, STRLEN, "%4.2f", (double) ImageInfo.ExposureBias);
    mapPut(env, map, "Exposure Bias", str);
  }

  mapPut(env, map, "White Balance", ImageInfo.Whitebalance ? "Manual" : "Auto");

  char *lightSource = NULL;
  switch (ImageInfo.LightSource) {
  case 1:
    lightSource = "Daylight";
    break;
  case 2:
    lightSource = "Fluorescent";
    break;
  case 3:
    lightSource = "Incandescent";
    break;
  case 4:
    lightSource = "Flash";
    break;
  case 9:
    lightSource = "Fine weather";
    break;
  case 11:
    lightSource = "Shade";
    break;
  }
  mapPut(env, map, "Light Source", lightSource);

  if (ImageInfo.MeteringMode > 0) {
    char *meteringMode = NULL;
    switch (ImageInfo.MeteringMode) {
    case 1:
      meteringMode = "Average";
      break;
    case 2:
      meteringMode = "Center Weighted";
      break;
    case 3:
      meteringMode = "Spot";
      break;
    case 4:
      meteringMode = "Multi-spot";
      break;
    case 5:
      meteringMode = "Pattern";
      break;
    case 6:
      meteringMode = "Partial";
      break;
    case 255:
      meteringMode = "Other";
      break;
    default:
      snprintf(str, STRLEN, "Unknown (%d)", ImageInfo.MeteringMode);
      meteringMode = str;
      break;
    }
    mapPut(env, map, "Metering Mode", meteringMode);
  }

  if (ImageInfo.ExposureProgram) {
    char *exposureProgram = NULL;
    switch (ImageInfo.ExposureProgram) {
    case 1:
      exposureProgram = "Manual";
      break;
    case 2:
      exposureProgram = "Program (auto)";
      break;
    case 3:
      exposureProgram = "Aperture-priority (semi-auto)";
      break;
    case 4:
      exposureProgram = "Shutter-priority (semi-auto)";
      break;
    case 5:
      exposureProgram = "Creative Program (based towards depth of field)";
      break;
    case 6:
      exposureProgram = "Action program (based towards fast shutter speed)\n";
      break;
    case 7:
      exposureProgram = "Portrait Mode";
      break;
    case 8:
      exposureProgram = "LandscapeMode";
      break;
    }
    mapPut(env, map, "Exposure", exposureProgram);
  }

  if (ImageInfo.ExposureMode) {
    mapPut(env, map, "Exposure Mode",
        ImageInfo.ExposureMode == 1 ? "Manual" : " Auto bracketing");
  }

  if (ImageInfo.ExposureMode) {
    char *exposureMode = NULL;
    switch (ImageInfo.ExposureMode) {
    case 1:
      exposureMode = "Manual";
      break;
    case 2:
      exposureMode = "Auto bracketing";
      break;
    }
    mapPut(env, map, "Exposure Mode", exposureMode);
  }

  if (ImageInfo.DistanceRange) {
    char *focusRange = NULL;
    switch (ImageInfo.DistanceRange) {
    case 1:
      focusRange = "Macro";
      break;
    case 2:
      focusRange = "Close";
      break;
    case 3:
      focusRange = "Distant";
      break;
    }
    mapPut(env, map, "Focus Range", focusRange);
  }

  char* jpegProcess = NULL;
  switch (ImageInfo.Process) {
  case M_SOF0:
    jpegProcess = "Baseline";
    break;
  case M_SOF1:
    jpegProcess = "Extended sequential";
    break;
  case M_SOF2:
    jpegProcess = "Progressive";
    break;
  case M_SOF3:
    jpegProcess = "Lossless";
    break;
  case M_SOF5:
    jpegProcess = "Differential sequential";
    break;
  case M_SOF6:
    jpegProcess = "Differential progressive";
    break;
  case M_SOF7:
    jpegProcess = "Differential lossless";
    break;
  case M_SOF9:
    jpegProcess = "Extended sequential, arithmetic coding";
    break;
  case M_SOF10:
    jpegProcess = "Progressive, arithmetic coding";
    break;
  case M_SOF11:
    jpegProcess = "Lossless, arithmetic coding";
    break;
  case M_SOF13:
    jpegProcess = "Differential sequential, arithmetic coding";
    break;
  case M_SOF14:
    jpegProcess = "Differential progressive, arithmetic coding";
    break;
  case M_SOF15:
    jpegProcess = "Differential lossless, arithmetic coding";
    break;
  default:
    jpegProcess = "Unknown";
    break;
  }
  mapPut(env, map, "JPEG Process", jpegProcess);

  if (ImageInfo.GpsInfoPresent) {
    mapPut(env, map, "GPS Latitude", ImageInfo.GpsLat);
    mapPut(env, map, "GPS Longitude", ImageInfo.GpsLong);
    mapPut(env, map, "GPS Altitude", ImageInfo.GpsAlt);
  }

  if (ImageInfo.QualityGuess) {
    snprintf(str, STRLEN, "%d", ImageInfo.QualityGuess);
    mapPut(env, map, "JPEG Quality", str);
  }

  if (ImageInfo.Comments[0]) {
    char *comments = ImageInfo.Comments;
    char temp[MAX_COMMENT_SIZE];
    if (ImageInfo.CommentWidthchars) {
      snprintf(temp, MAX_COMMENT_SIZE, "%.*ls", ImageInfo.CommentWidthchars,
          (wchar_t *) ImageInfo.Comments);
      comments = temp;
    }
    mapPut(env, map, "Comments", comments);
  }

  LOGD("Got image info for %s", filename);
  done:
  (*env)->ReleaseStringUTFChars(env, jfilename, filename);
  DiscardData();
  return map;
}

static const JNINativeMethod METHOD_TABLE[] = {
  { "getThumbnail", "(Ljava/lang/String;)[B", (void *) getThumbnail },
  { "getImageInfo", "(Ljava/lang/String;)Ljava/util/Map;", (void *) getImageInfo },
};

static const int METHOD_TABLE_SIZE = sizeof(METHOD_TABLE)
    / sizeof(METHOD_TABLE[0]);

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
  JNIEnv* env;
  if ((*vm)->GetEnv(vm, (void **) &env, JNI_VERSION_1_6) == JNI_OK) {
    jclass clazz = (*env)->FindClass(env, "com/twitter/university/exify/JHead");
    if (clazz) {
      jint ret = (*env)->RegisterNatives(env, clazz, METHOD_TABLE,
          METHOD_TABLE_SIZE);
      (*env)->DeleteLocalRef(env, clazz);
      LOGD("Registered native methods");
      if (ret == 0) {
        ret = initGlobals(env);
        if (ret == 0) {
          LOGD("JNI_OnLoad(...) succeess");
          return JNI_VERSION_1_6;
        }
      }
    }
  }
  LOGD("JNI_OnLoad(...) failure");
  return JNI_ERR;
}
