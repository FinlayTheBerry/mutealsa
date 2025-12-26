#define _GNU_SOURCE
#include <alsa/asoundlib.h>

int main(int argc, char **argv)
{
    int muteState = 0;
    if (argc >= 2 && strcmp(argv[1], "post") == 0)
    {
        muteState = 1;
    }

    int error_code = 0;
    int card_id = -1;
    while (1)
    {
        char *card_name = NULL;
        char *card_human_name = NULL;
        snd_mixer_t *mixer = NULL;
        snd_ctl_t *ctl = NULL;
        if ((error_code = snd_card_next(&card_id)) != 0)
        {
            printf("Audio error - snd_card_next failed - %s\n", snd_strerror(error_code));
            fflush(stdout);
            goto cleanup_card;
        }
        if (card_id < 0)
        {
            break;
        }

        card_name = (char *)malloc(snprintf(NULL, 0, "hw:%d", card_id) + 1);
        sprintf(card_name, "hw:%d", card_id);
        if ((error_code = snd_card_get_name(card_id, &card_human_name)) != 0)
        {
            const char unknown_card_name[] = "Unnamed sound card";
            card_human_name = malloc(sizeof(unknown_card_name));
            memcpy(card_human_name, unknown_card_name, sizeof(unknown_card_name));
        }
        printf("Audio info - Found sound card %s at %s\n", card_human_name, card_name);
        fflush(stdout);

        if ((error_code = snd_mixer_open(&mixer, 0)) != 0)
        {
            printf("Audio error - snd_mixer_open failed - %s\n", snd_strerror(error_code));
            fflush(stdout);
            goto cleanup_card;
        }
        if ((error_code = snd_mixer_attach(mixer, card_name)) != 0)
        {
            printf("Audio error - snd_mixer_attach failed - %s\n", snd_strerror(error_code));
            fflush(stdout);
            goto cleanup_card;
        }
        if ((error_code = snd_mixer_selem_register(mixer, NULL, NULL)) != 0)
        {
            printf("Audio error - snd_mixer_selem_register failed - %s\n", snd_strerror(error_code));
            fflush(stdout);
            goto cleanup_card;
        }
        if ((error_code = snd_mixer_load(mixer)) != 0)
        {
            printf("Audio error - snd_mixer_load failed - %s\n", snd_strerror(error_code));
            fflush(stdout);
            goto cleanup_card;
        }
        // elem is a pointer to mixer data and doesn't need to be freed. It's freed along with the mixer.
        snd_mixer_elem_t *elem = NULL;
        for (elem = snd_mixer_first_elem(mixer); elem; elem = snd_mixer_elem_next(elem))
        {
            const char *elem_name = snd_mixer_selem_get_name(elem);
            int elem_index = snd_mixer_selem_get_index(elem);
            if (snd_mixer_elem_get_type(elem) != SND_MIXER_ELEM_SIMPLE) {
                printf("WARN NON SIMPLE %s has control %s,%d\n", card_name, elem_name, elem_index);
                fflush(stdout);
            }
            if (snd_mixer_selem_has_playback_switch(elem))
            {
                if (muteState == 0)
                {
                    printf("Audio Info - %s has control %s,%d with mute toggle. Muting...\n", card_name, elem_name, elem_index);
                    fflush(stdout);
                }
                else
                {
                    printf("Audio Info - %s has control %s,%d with mute toggle. Unmuting...\n", card_name, elem_name, elem_index);
                    fflush(stdout);
                }
                if ((error_code = snd_mixer_selem_set_playback_switch_all(elem, muteState)) != 0)
                {
                    printf("Audio error - snd_mixer_selem_set_playback_switch_all failed - %s\n", snd_strerror(error_code));
                    fflush(stdout);
                }
            }
        }

    cleanup_card:
        if (card_name != NULL)
        {
            free(card_name);
        }
        if (card_human_name != NULL)
        {
            free(card_human_name);
        }
        if (mixer != NULL)
        {
            snd_mixer_close(mixer);
        }
        if (ctl != NULL)
        {
            snd_ctl_close(ctl);
        }
    }
}