#ifndef WAV_H
#define WAV_H
#include <sndfile.h>

#define FRAMES_PER_BUFFER   (512)

typedef struct
{
    SNDFILE     *file;
    SF_INFO      info;
} callback_data_s;

void do_alert(char *wav_path);
void dismiss();

#endif
