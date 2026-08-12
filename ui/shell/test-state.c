#include "mantle-shell-state.h"

#include <stdio.h>
#include <string.h>

int main(void) {
    struct mantle_shell_state state;
    char json[256];
    mantle_shell_state_init(&state);
    if (mantle_shell_state_set_workspace(&state, 2) != 0 || mantle_shell_state_set_workspace(&state, 0) == 0) return 1;
    mantle_shell_state_set_network(&state, 1);
    mantle_shell_state_set_audio_muted(&state, 1);
    mantle_shell_state_set_battery(&state, 101);
    unsigned id = mantle_shell_state_notify(&state, "Réseau", "Connexion active");
    if (id == 0 || state.battery_percent != 100 || state.notification_count != 1) return 1;
    if (mantle_shell_state_json(&state, json, sizeof(json)) < 0 || !strstr(json, "\"network\":true")) return 1;
    if (mantle_shell_state_dismiss(&state, id) != 0 || state.notification_count != 0) return 1;
    puts("Mantle shell state checks: OK");
    return 0;
}
