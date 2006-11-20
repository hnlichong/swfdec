#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <string.h>
#include <libswfdec/swfdec.h>

static gboolean
audio_diff (SwfdecBuffer *compare, SwfdecBuffer *original)
{
  guint i;
  gint16 *comp_data, *org_data;
  
  if (compare->length != original->length) {
    g_print ("  ERROR: lengths don't match (is %u, should be %u)\n", 
	compare->length, original->length);
    return FALSE;
  }
  comp_data = (gint16 *) compare->data;
  org_data = (gint16 *) original->data;
  for (i = 0; i < compare->length / 2; i++) {
    if (comp_data[i] != org_data[i]) {
      g_print ("  ERROR: data mismatch at sample offset %u (is 0x%04X, should be 0x%04X)\n",
	  i, (gint) comp_data[i], (gint) org_data[i]);
      return FALSE;
    }
  }
  return TRUE;
}

typedef struct {
  SwfdecAudio *		audio;
  char *		name;
  SwfdecBufferQueue *	queue;
} TestStream;

typedef struct {
  const char *	filename;
  GList *	files;
  GList *	streams;
  guint		current_frame;
  guint		current_frame_audio;
  gboolean	success;
} TestData;

static void
audio_added (SwfdecPlayer *player, SwfdecAudio *audio, TestData *data)
{
  char *name = g_strdup_printf ("%s.%u.%u.raw", data->filename, data->current_frame, data->current_frame_audio);
  GList *found = g_list_find_custom (data->files, name, (GCompareFunc) strcmp);
  if (found == NULL) {
    g_print ("  ERROR: %s wasn't found\n", name);
    data->success = FALSE;
  } else {
    TestStream *stream = g_new0 (TestStream, 1);
    stream->audio = audio;
    stream->name = found->data;
    stream->queue = swfdec_buffer_queue_new ();
    data->files = g_list_delete_link (data->files, found);
    data->streams = g_list_prepend (data->streams, stream);
  }
  g_free (name);
}

static gboolean
finish_stream (TestStream *stream)
{
  SwfdecBuffer *buffer, *file;
  GError *error = NULL;
  gboolean ret = TRUE;

  buffer = swfdec_buffer_queue_pull (stream->queue, swfdec_buffer_queue_get_depth (stream->queue));
  swfdec_buffer_queue_free (stream->queue);
  file = swfdec_buffer_new_from_file (stream->name, &error);
  if (file) {
    ret = audio_diff (buffer, file);
    swfdec_buffer_unref (file);
  } else {
    g_print ("  ERROR: %s\n", error->message);
    g_error_free (error);
    ret = FALSE;
  }
  swfdec_buffer_unref (buffer);
  g_free (stream->name);
  g_free (stream);
  return ret;
}

static void
audio_removed (SwfdecPlayer *player, SwfdecAudio *audio, TestData *data)
{
  TestStream *stream;
  GList *walk;

  for (walk = data->streams; walk; walk = walk->next) {
    stream = walk->data;
    if (stream->audio == audio)
      break;
    stream = NULL;
  }
  if (stream) {
    data->streams = g_list_remove (data->streams, stream);
    data->success &= finish_stream (stream);
  }
}

static gboolean
run_test (const char *filename)
{
  SwfdecLoader *loader;
  SwfdecPlayer *player = NULL;
  guint i;
  GError *error = NULL;
  char *dirname;
  const char *name;
  GDir *dir;
  GList *walk;
  TestData data = { filename, NULL, 0, 0, TRUE };

  g_print ("Testing %s:\n", filename);
  dirname = g_path_get_dirname (filename);
  dir = g_dir_open (dirname, 0, &error);
  if (!dir) {
    g_print ("  ERROR: %s\n", error->message);
    g_object_unref (player);
    return FALSE;
  }
  while ((name = g_dir_read_name (dir))) {
    if (!g_str_has_prefix (name, filename))
      continue;
    if (!g_str_has_suffix (name, ".raw"))
      continue;
    data.files = g_list_prepend (data.files, g_strdup (name));
  }
  g_dir_close (dir);

  loader = swfdec_loader_new_from_file (filename, &error);
  if (loader == NULL) {
    g_print ("  ERROR: %s\n", error->message);
    goto error;
  }
  player = swfdec_player_new ();
  g_signal_connect (player, "audio-added", G_CALLBACK (audio_added), &data);
  g_signal_connect (player, "audio-removed", G_CALLBACK (audio_removed), &data);
  swfdec_player_set_loader (player, loader);

  for (i = 0; i < 10; i++) {
    data.current_frame++;
    data.current_frame_audio = 0;
    swfdec_player_iterate (player);
  }
  g_object_unref (player);
  for (walk = data.streams; walk; walk = walk->next) {
    data.success &= finish_stream (walk->data);
  }
  g_list_free (data.streams);
  if (data.files) {
    g_print ("  ERROR: streams not played:\n");
    for (walk = data.files; walk; walk = walk->next) {
      g_print ("         %s\n", (char *) walk->data);
      g_free (walk->data);
    }
    g_list_free (data.files);
  }
  if (data.success) {
    g_print ("  OK\n");
    return TRUE;
  } else {
    return FALSE;
  }

error:
  if (error)
    g_error_free (error);
  if (player)
    g_object_unref (player);
  g_list_foreach (data.files, (GFunc) g_free, NULL);
  g_list_free (data.files);
  return FALSE;
}

int
main (int argc, char **argv)
{
  guint failed_tests = 0;

  swfdec_init ();

  if (argc > 1) {
    int i;
    for (i = 1; i < argc; i++) {
      if (!run_test (argv[i]))
	failed_tests++;
    }
  } else {
    GDir *dir;
    const char *file;
    dir = g_dir_open (".", 0, NULL);
    while ((file = g_dir_read_name (dir))) {
      if (!g_str_has_suffix (file, ".swf"))
	continue;
      if (!run_test (file))
	failed_tests++;
    }
    g_dir_close (dir);
  }

  if (failed_tests) {
    g_print ("\nFAILURES: %u\n", failed_tests);
  } else {
    g_print ("\nEVERYTHING OK\n");
  }
  return failed_tests;
}

