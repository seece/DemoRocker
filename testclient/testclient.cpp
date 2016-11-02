#include <sync.h>

#include <cstdio>
#include <cstdlib>
#include <cmath>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static void die(const char *fmt, ...)
{
	char temp[4096];
	va_list va;
	va_start(va, fmt);
	vsnprintf(temp, sizeof(temp), fmt, va);
	va_end(va);

#if !defined(_WIN32) || defined(_CONSOLE)
	fprintf(stderr, "*** error: %s\n", temp);
#else
	MessageBox(NULL, temp, NULL, MB_OK | MB_ICONERROR);
#endif

	exit(EXIT_FAILURE);
}

static const float bpm = 150.0f; /* beats per minute */
static const int rpb = 8; /* rows per beat */
static const double row_rate = (double(bpm) / 60) * rpb;

static double playback_row = 0.0;
static bool song_playing = false;

static void callback_pause(void *d, int flag)
{
	song_playing = !flag;
	printf("%s %d\n", __FUNCTION__, flag);
}

static void callback_set_row(void *d, int row)
{
	playback_row = row;
	printf("%s %d\n", __FUNCTION__, row);
}

static int callback_is_playing(void *d)
{
	puts(__FUNCTION__);
	return song_playing;
}

static struct sync_cb callbacks = {
	callback_pause,
	callback_set_row,
	callback_is_playing
};

int main(int argc, char* argv[])
{
	puts("starting");

	sync_device *rocket = sync_create_device("sync");
	if (!rocket)
		die("out of memory?");

	if (sync_tcp_connect(rocket, "localhost", SYNC_DEFAULT_PORT))
		die("failed to connect to host");

	const sync_track *track_a1 = sync_get_track(rocket, "a:1");
	const sync_track *track_a2 = sync_get_track(rocket, "a:2");
	const sync_track *track_b = sync_get_track(rocket, "b");
	const sync_track *track_c = sync_get_track(rocket, "c");

	bool done = false;
	while (!done) {
		if (sync_update(rocket, (int)floor(playback_row), &callbacks, NULL)) {
			sync_tcp_connect(rocket, "localhost", SYNC_DEFAULT_PORT);
		}

		double a1 = sync_get_val(track_a1, playback_row);
		double a2 = sync_get_val(track_a2, playback_row);
		double b = sync_get_val(track_b, playback_row);
		double c = sync_get_val(track_c, playback_row);
		printf("a1, a2, b, c: %f, %f, %f, %f\n", a1, a2, b, c);
		if (song_playing) {
			playback_row += 0.5;
		}
		Sleep(200);
	}

	sync_save_tracks(rocket);
	sync_destroy_device(rocket);

	return 0;
}