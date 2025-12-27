#define _GNU_SOURCE
#include <alsa/asoundlib.h>

const char *state_path = "/run/sleepmute-muted-60eaa8c5-be35-4e04-a9fa-4616caadb819";

#include <sys/mman.h>
void bootstrap_alsa() {
    const char *mini_alsa_conf = "ctl.hw { @args [ CARD ] @args.CARD { type string } type hw card $CARD }";
    int fd = memfd_create("mini_alsa_conf", 0);
    write(fd, mini_alsa_conf, strlen(mini_alsa_conf));
    char fd_path[25];
    sprintf(fd_path, "/proc/self/fd/%d", fd);
    setenv("ALSA_CONFIG_PATH", fd_path, 1);
}

void cleanup(char **card_name, char **elem_id_str, snd_mixer_t **mixer)
{
    if (*card_name != NULL)
    {
        free(*card_name);
        *card_name = NULL;
    }
    if (*elem_id_str != NULL)
    {
        free(*elem_id_str);
        *elem_id_str = NULL;
    }
    if (*mixer != NULL)
    {
        snd_mixer_close(*mixer);
        *mixer = NULL;
    }
}

int main(int argc, char **argv)
{
    bootstrap_alsa();

    int error_code = 0;
    char *card_name = NULL;
    char *elem_id_str = NULL;
    snd_mixer_t *mixer = NULL;

    int restore = 0;
    if (argc >= 2 && strcmp(argv[1], "pre") == 0)
    {
        restore = 0;
    }
    else if (argc >= 2 && strcmp(argv[1], "post") == 0)
    {
        restore = 1;
    }
    else
    {
        fprintf(stderr, "Usage: sleepmute (pre|post)\n");
        fflush(stderr);
        cleanup(&card_name, &elem_id_str, &mixer);
        return 1;
    }

    if (!restore)
    {
        FILE *f = fopen(state_path, "w");
        if (f) {
            fclose(f);
        }
    }

    int card_id = -1;
    while (1)
    {
        if ((error_code = snd_card_next(&card_id)) != 0)
        {
            fprintf(stderr, "error - snd_card_next - %s\n", snd_strerror(error_code));
            fflush(stderr);
            cleanup(&card_name, &elem_id_str, &mixer);
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
            cleanup(&card_name, &elem_id_str, &mixer);
            return 1;
        }
        sprintf(card_name, "hw:%d", card_id);

        if ((error_code = snd_card_get_longname(card_id, &card_name)) != 0)
        {
            fprintf(stderr, "error - snd_card_get_longname - %s\n", snd_strerror(error_code));
            fflush(stderr);
            cleanup(&card_name, &elem_id_str, &mixer);
            return 1;
        }

        if ((error_code = snd_mixer_open(&mixer, 0)) != 0)
        {
            fprintf(stderr, "error - snd_mixer_open - %s\n", snd_strerror(error_code));
            fflush(stderr);
            cleanup(&card_name, &elem_id_str, &mixer);
            return 1;
        }

        
        char card_id_str[14]; // Just long enough for "hw:4294967296\0"
        sprintf(card_id_str, "hw:%d", card_id);
        if ((error_code = snd_mixer_attach(mixer, card_id_str)) != 0)
        {
            fprintf(stderr, "error - snd_mixer_attach - %s\n", snd_strerror(error_code));
            fflush(stderr);
            cleanup(&card_name, &elem_id_str, &mixer);
            return 1;
        }

        if ((error_code = snd_mixer_selem_register(mixer, NULL, NULL)) != 0)
        {
            fprintf(stderr, "error - snd_mixer_selem_register - %s\n", snd_strerror(error_code));
            fflush(stderr);
            cleanup(&card_name, &elem_id_str, &mixer);
            return 1;
        }

        if ((error_code = snd_mixer_load(mixer)) != 0)
        {
            fprintf(stderr, "error - snd_mixer_load - %s\n", snd_strerror(error_code));
            fflush(stderr);
            cleanup(&card_name, &elem_id_str, &mixer);
            return 1;
        }

        for (snd_mixer_elem_t *elem = snd_mixer_first_elem(mixer); elem != NULL; elem = snd_mixer_elem_next(elem))
        {
            if (!snd_mixer_selem_has_playback_switch(elem))
            {
                continue;
            }

            unsigned int elem_id = snd_mixer_selem_get_index(elem);
            const char *elem_name = snd_mixer_selem_get_name(elem);

            for (int i = 0; i <= SND_MIXER_SCHN_LAST; i++)
            {
                if (!snd_mixer_selem_has_playback_channel(elem, i))
                {
                    continue;
                }

                elem_id_str = malloc(snprintf(NULL, 0, "%d,%s,%d,%s,%d", card_id, card_name, elem_id, elem_name, i) + 1);
                if (elem_id_str == NULL)
                {
                    fprintf(stderr, "error - malloc - out of memory\n");
                    fflush(stderr);
                    cleanup(&card_name, &elem_id_str, &mixer);
                    return 1;
                }
                sprintf(elem_id_str, "%d,%s,%d,%s,%d", card_id, card_name, elem_id, elem_name, i);

                if (restore)
                {
                    int muteState = 1;
                    FILE *f = fopen(state_path, "r");
                    if (f)
                    {
                        char line[1024];
                        while (fgets(line, sizeof(line), f))
                        {
                            line[strcspn(line, "\n")] = 0;
                            if (strcmp(line, elem_id_str) == 0)
                            {
                                muteState = 0;
                                break;
                            }
                        }
                        fclose(f);
                    }

                    if ((error_code = snd_mixer_selem_set_playback_switch(elem, i, muteState)) != 0)
                    {
                        fprintf(stderr, "error - snd_mixer_selem_set_playback_switch - %s\n", snd_strerror(error_code));
                        fflush(stderr);
                        cleanup(&card_name, &elem_id_str, &mixer);
                        return 1;
                    }
                }
                else
                {
                    int value = 0;
                    if ((error_code = snd_mixer_selem_get_playback_switch(elem, i, &value)) != 0)
                    {
                        fprintf(stderr, "error - snd_mixer_selem_get_playback_switch - %s\n", snd_strerror(error_code));
                        fflush(stderr);
                        cleanup(&card_name, &elem_id_str, &mixer);
                        return 1;
                    }

                    if (value == 0)
                    {
                        FILE *f = fopen(state_path, "a");
                        if (f)
                        {
                            fprintf(f, "%s\n", elem_id_str);
                            fclose(f);
                        }
                    }

                    if ((error_code = snd_mixer_selem_set_playback_switch(elem, i, 0)) != 0)
                    {
                        fprintf(stderr, "error - snd_mixer_selem_set_playback_switch - %s\n", snd_strerror(error_code));
                        fflush(stderr);
                        cleanup(&card_name, &elem_id_str, &mixer);
                        return 1;
                    }
                }
            }
        }
        cleanup(&card_name, &elem_id_str, &mixer);
    }
    return 0;
}