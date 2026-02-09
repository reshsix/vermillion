#include <vermillion/entry.h>

extern bool
vrm_entry(struct vrm *v, const char **args, int count)
{
    const char msg[] = "vrm_entry message\r\n";
    v->vars.set("msg", msg);

    const char *msg2 = v->vars.get("msg");
    for (int i = 0; i < sizeof(msg) - 1; i++)
        v->comm.write0(msg2[i]);

    return true;
}
