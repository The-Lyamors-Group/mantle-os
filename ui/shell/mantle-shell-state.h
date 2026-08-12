#ifndef MANTLE_SHELL_STATE_H
#define MANTLE_SHELL_STATE_H

#include <stddef.h>

#define MANTLE_SHELL_MAX_WORKSPACES 9
#define MANTLE_SHELL_MAX_NOTIFICATIONS 32

struct mantle_shell_notification { unsigned id; char title[96]; char body[256]; int unread; };
struct mantle_shell_state {
    unsigned workspace_count;
    unsigned current_workspace;
    int network_online;
    int audio_muted;
    int battery_percent;
    unsigned focused_window;
    unsigned next_notification_id;
    unsigned notification_count;
    struct mantle_shell_notification notifications[MANTLE_SHELL_MAX_NOTIFICATIONS];
};

void mantle_shell_state_init(struct mantle_shell_state *state);
int mantle_shell_state_set_workspace(struct mantle_shell_state *state, unsigned workspace);
void mantle_shell_state_set_network(struct mantle_shell_state *state, int online);
void mantle_shell_state_set_audio_muted(struct mantle_shell_state *state, int muted);
void mantle_shell_state_set_battery(struct mantle_shell_state *state, int percent);
unsigned mantle_shell_state_notify(struct mantle_shell_state *state, const char *title, const char *body);
int mantle_shell_state_dismiss(struct mantle_shell_state *state, unsigned id);
int mantle_shell_state_json(const struct mantle_shell_state *state, char *out, size_t size);

#endif
