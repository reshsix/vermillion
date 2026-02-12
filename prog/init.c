#include <vermillion/entry.h>

static char line[1024] = {0};
static size_t line_c = 0;

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

            for (int i = 0; line[i] != '\0'; i++)
                v->comm.write0(line[i]);
            v->comm.write0('\r');
            v->comm.write0('\n');

            line_c = 0;
            v->comm.write0('>');
            v->comm.write0(' ');
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
