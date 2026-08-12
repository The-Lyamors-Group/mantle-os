#include "mantle-shell-state.h"

#include <stdio.h>
#include <string.h>

void mantle_shell_state_init(struct mantle_shell_state *state) {
    memset(state, 0, sizeof(*state));
    state->workspace_count = 4;
    state->current_workspace = 1;
    state->battery_percent = -1;
    state->next_notification_id = 1;
}

int mantle_shell_state_set_workspace(struct mantle_shell_state *state, unsigned workspace) {
    if (workspace == 0 || workspace > state->workspace_count) return -1;
    state->current_workspace = workspace;
    return 0;
}

void mantle_shell_state_set_network(struct mantle_shell_state *state, int online) { state->network_online = online != 0; }
void mantle_shell_state_set_audio_muted(struct mantle_shell_state *state, int muted) { state->audio_muted = muted != 0; }
void mantle_shell_state_set_battery(struct mantle_shell_state *state, int percent) {
    state->battery_percent = percent < 0 ? -1 : percent > 100 ? 100 : percent;
}

unsigned mantle_shell_state_notify(struct mantle_shell_state *state, const char *title, const char *body) {
    if (state->notification_count == MANTLE_SHELL_MAX_NOTIFICATIONS) {
        memmove(&state->notifications[0], &state->notifications[1], sizeof(state->notifications[0]) * (MANTLE_SHELL_MAX_NOTIFICATIONS - 1));
        state->notification_count--;
    }
    struct mantle_shell_notification *notification = &state->notifications[state->notification_count++];
    notification->id = state->next_notification_id++;
    snprintf(notification->title, sizeof(notification->title), "%s", title ? title : "MantleOS");
    snprintf(notification->body, sizeof(notification->body), "%s", body ? body : "");
    notification->unread = 1;
    return notification->id;
}

int mantle_shell_state_dismiss(struct mantle_shell_state *state, unsigned id) {
    for (unsigned i = 0; i < state->notification_count; i++) {
        if (state->notifications[i].id != id) continue;
        memmove(&state->notifications[i], &state->notifications[i + 1], sizeof(state->notifications[0]) * (state->notification_count - i - 1));
        state->notification_count--;
        return 0;
    }
    return -1;
}

int mantle_shell_state_json(const struct mantle_shell_state *state, char *out, size_t size) {
    int written = snprintf(out, size, "{\"workspace\":%u,\"workspaces\":%u,\"network\":%s,\"muted\":%s,\"battery\":%d,\"notifications\":%u}",
        state->current_workspace, state->workspace_count, state->network_online ? "true" : "false",
        state->audio_muted ? "true" : "false", state->battery_percent, state->notification_count);
    return written < 0 || (size_t)written >= size ? -1 : written;
}
