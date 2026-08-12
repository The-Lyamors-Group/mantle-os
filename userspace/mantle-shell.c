#include "libc/mantle.h"

static void output(const char *text)
{
    mantle_write(1u, text, mantle_strlen(text));
}

static void output_line(const char *text)
{
    output(text);
    output("\n");
}

static void execute_line(char *line)
{
    uint64_t length = mantle_strlen(line);
    char *argument;

    while (length > 0u && (line[length - 1u] == '\n' || line[length - 1u] == '\r')) {
        line[--length] = '\0';
    }
    if (mantle_streq(line, "")) {
        return;
    }
    if (mantle_streq(line, "exit")) {
        mantle_exit(0);
    }
    if (mantle_streq(line, "pwd")) {
        char cwd[64];
        if (mantle_getcwd(cwd, sizeof(cwd)) >= 0) {
            output_line(cwd);
        }
        return;
    }
    if (mantle_streq(line, "echo MantleOS")) {
        output_line("MantleOS");
        return;
    }
    if (mantle_streq(line, "ls")) {
        char entries[128];
        if (mantle_readdir("/", entries, sizeof(entries)) >= 0) {
            output(entries);
        } else {
            output_line("mantle-shell: directory unavailable");
        }
        return;
    }
    if (mantle_streq(line, "uname")) {
        char name[32];
        if (mantle_uname(name, sizeof(name)) >= 0) {
            output_line(name);
        }
        return;
    }
    if (mantle_streq(line, "mantle --version")) {
        if (mantle_exec("/bin/mantle") < 0) {
            output_line("mantle-shell: exec failed");
        }
        return;
    }
    if (mantle_streq(line, "/bin/hello")) {
        if (mantle_exec("/bin/hello") < 0) {
            output_line("mantle-shell: exec failed");
        }
        return;
    }
    if (line[0] == 'c' && line[1] == 'd' && line[2] == ' ') {
        argument = line + 3;
        if (mantle_chdir(argument) < 0) {
            output_line("mantle-shell: directory unavailable");
        }
        return;
    }
    output_line("mantle-shell: command not found");
}

int main(void)
{
    char line[128];
    static const char marker[] = "MANTLE_SHELL_CONSOLE_OK\n";
    static const char prompt[] = "mantle@mantleos:~$ ";
    uint64_t length;

    mantle_write(1u, marker, sizeof(marker) - 1u);
    for (;;) {
        output(prompt);
        length = (uint64_t)mantle_read(0u, line, sizeof(line) - 1u);
        if ((int64_t)length < 0) {
            return 1;
        }
        line[length < sizeof(line) ? length : sizeof(line) - 1u] = '\0';
        execute_line(line);
    }
}
