#define _GNU_SOURCE
#include <alsa/asoundlib.h>

void cleanup(char **card_name, snd_mixer_t **mixer)
{
    if (*card_name != NULL)
    {
        free(*card_name);
        *card_name = NULL;
    }
    if (*mixer != NULL)
    {
        snd_mixer_close(*mixer);
        *mixer = NULL;
    }
}

struct state {
    int card_id;
    int elem_id;
    int state;
};

int main(int argc, char **argv)
{
    //const char* state_path = "/run/mutealsa-state-60eaa8c5-be35-4e04-a9fa-4616caadb819";

    int error_code = 0;
    int muteState = 0;
    int card_id = -1;
    char *card_name = NULL;
    snd_mixer_t *mixer = NULL;
    // elem is a pointer to mixer data and doesn't need to be freed. It's freed along with the mixer.
    snd_mixer_elem_t *elem = NULL;

    if (argc >= 2 && strcmp(argv[1], "pre") == 0)
    {
        muteState = 0;
    }
    else if (argc >= 2 && strcmp(argv[1], "post") == 0)
    {
        muteState = 1;
    }
    else
    {
        fprintf(stderr, "Usage: mutealsa (pre|post)\n");
        fflush(stderr);
        cleanup(&card_name, &mixer);
        return 1;
    }

    while (1)
    {
        if ((error_code = snd_card_next(&card_id)) != 0)
        {
            fprintf(stderr, "error - snd_card_next - %s\n", snd_strerror(error_code));
            fflush(stderr);
            cleanup(&card_name, &mixer);
            return 1;
        }
        if (card_id < 0)
        {
            break;
        }

        card_name = (char *)malloc(snprintf(NULL, 0, "hw:%d", card_id) + 1);
        if (card_name == NULL)
        {
            fprintf(stderr, "error - malloc - out of memory\n");
            fflush(stderr);
            cleanup(&card_name, &mixer);
            return 1;
        }
        sprintf(card_name, "hw:%d", card_id);

        if ((error_code = snd_mixer_open(&mixer, 0)) != 0)
        {
            fprintf(stderr, "error - snd_mixer_open - %s\n", snd_strerror(error_code));
            fflush(stderr);
            cleanup(&card_name, &mixer);
            return 1;
        }

        if ((error_code = snd_mixer_attach(mixer, card_name)) != 0)
        {
            fprintf(stderr, "error - snd_mixer_attach - %s\n", snd_strerror(error_code));
            fflush(stderr);
            cleanup(&card_name, &mixer);
            return 1;
        }

        if ((error_code = snd_mixer_selem_register(mixer, NULL, NULL)) != 0)
        {
            fprintf(stderr, "error - snd_mixer_selem_register - %s\n", snd_strerror(error_code));
            fflush(stderr);
            cleanup(&card_name, &mixer);
            return 1;
        }

        if ((error_code = snd_mixer_load(mixer)) != 0)
        {
            fprintf(stderr, "error - snd_mixer_load - %s\n", snd_strerror(error_code));
            fflush(stderr);
            cleanup(&card_name, &mixer);
            return 1;
        }

        for (elem = snd_mixer_first_elem(mixer); elem; elem = snd_mixer_elem_next(elem))
        {
            int elem_id = snd_mixer_selem_get_index(elem);
            printf("Card %d elem %d\n", card_id, elem_id);
            fflush(stdout);

            if (!snd_mixer_selem_has_playback_switch(elem))
            {
                continue;
            }
            if ((error_code = snd_mixer_selem_set_playback_switch_all(elem, muteState)) != 0)
            {
                fprintf(stderr, "error - snd_mixer_selem_set_playback_switch_all - %s\n", snd_strerror(error_code));
                fflush(stderr);
                cleanup(&card_name, &mixer);
                return 1;
            }
        }

        cleanup(&card_name, &mixer);
    }
    return 0;
}