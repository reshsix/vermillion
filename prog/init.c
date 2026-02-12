#include <vermillion/entry.h>

static char line[1024] = {0};
static size_t line_c = 0;

static char *list[256] = {0};
static size_t list_c = 0;

static char buffer[2048] = {0};

extern bool
vrm_entry(struct vrm *v, const char **args, int count)
{
    (void)args;
    (void)count;

    v->comm.write0('>');
    v->comm.write0(' ');

    while (true)
    {
        char c = v->comm.read0();
        if (c == '\r')
        {
            line[line_c++] = '\0';
            v->comm.write0('\r');
            v->comm.write0('\n');

            /* Parse program */
            char *saveptr = NULL;
            list[0] = v->str.token(line, " ", &saveptr);
            if (list[0])
                list_c++;

            /* Parse args */
            for (size_t i = 0; list[i] && i < 255; i++)
            {
                list[i + 1] = v->str.token(NULL, " ", &saveptr);
                list_c++;
            }

            /* Make path */
            v->mem.fill(buffer, '\0', sizeof(buffer));
            v->str.copy(buffer, "/prog/", 6);
            v->str.concat(buffer, list[0], v->str.length(list[0]));
            v->str.concat(buffer, ".elf", 4);

            /* Run program */
            if (list_c)
            {
                uint32_t entry = 0;
                uint8_t *mem = v->loader.prog(buffer, &entry);
                if (mem)
                {
                    vrm_entry_t f = (void *)&(mem[entry]);
                    v->syslog.string(f(v, list, list_c) ? "Success" :
                                                          "Failure");
                }
                else
                    v->syslog.string("ERROR: Program not found\r\n");
                v->mem.del(mem);
            }

            /* Cleanup */
            for (size_t i = 0; i < list_c; i++)
                list[i] = NULL;
            line_c = 0;

            /* Prompt */
            v->comm.write0('>');
            v->comm.write0(' ');
        }
        else if (c == '\b' || c == 0x7F)
        {
            if (line_c > 0)
            {
                v->comm.write0('\b');
                v->comm.write0(' ');
                v->comm.write0('\b');
                line[--line_c] = '\0';
            }
        }
        else
        {
            if (line_c < sizeof(line))
            {
                v->comm.write0(c);
                line[line_c++] = c;
            }
            else
            {
                v->syslog.string("NOTICE: user is typing too much");
                break;
            }
        }
    }

    return false;
}
