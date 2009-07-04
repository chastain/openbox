#include "openbox/actions.h"
#include "openbox/screen.h"
#include "openbox/client.h"
#include <glib.h>

typedef enum {
    LAST,
    CURRENT,
    RELATIVE,
    ABSOLUTE
} SwitchType;

typedef struct {
    SwitchType type;
    union {
        struct {
            guint desktop;
        } abs;

        struct {
            gboolean linear;
            gboolean wrap;
            ObDirection dir;
        } rel;
    } u;
    gboolean send;
    gboolean follow;
} Options;

static gpointer setup_go_func(xmlNodePtr node);
static gpointer setup_send_func(xmlNodePtr node);
static gboolean run_func(ObActionsData *data, gpointer options);

void action_desktop_startup(void)
{
    actions_register("GoToDesktop", setup_go_func, g_free, run_func,
                     NULL, NULL);
    actions_register("SendToDesktop", setup_send_func, g_free, run_func,
                     NULL, NULL);
}

static gpointer setup_go_func(xmlNodePtr node)
{
    xmlNodePtr n;
    Options *o;

    o = g_new0(Options, 1);
    /* don't go anywhere if theres no options given */
    o->type = ABSOLUTE;
    o->u.abs.desktop = screen_desktop;
    /* wrap by default - it's handy! */
    o->u.rel.wrap = TRUE;

    if ((n = obt_parse_find_node(node, "to"))) {
        gchar *s = obt_parse_node_string(n);
        if (!g_ascii_strcasecmp(s, "last"))
            o->type = LAST;
        else if (!g_ascii_strcasecmp(s, "current"))
            o->type = CURRENT;
        else if (!g_ascii_strcasecmp(s, "next")) {
            o->type = RELATIVE;
            o->u.rel.linear = TRUE;
            o->u.rel.dir = OB_DIRECTION_EAST;
        }
        else if (!g_ascii_strcasecmp(s, "previous")) {
            o->type = RELATIVE;
            o->u.rel.linear = TRUE;
            o->u.rel.dir = OB_DIRECTION_WEST;
        }
        else if (!g_ascii_strcasecmp(s, "north") ||
                 !g_ascii_strcasecmp(s, "up")) {
            o->type = RELATIVE;
            o->u.rel.dir = OB_DIRECTION_NORTH;
        }
        else if (!g_ascii_strcasecmp(s, "south") ||
                 !g_ascii_strcasecmp(s, "down")) {
            o->type = RELATIVE;
            o->u.rel.dir = OB_DIRECTION_SOUTH;
        }
        else if (!g_ascii_strcasecmp(s, "west") ||
                 !g_ascii_strcasecmp(s, "left")) {
            o->type = RELATIVE;
            o->u.rel.dir = OB_DIRECTION_WEST;
        }
        else if (!g_ascii_strcasecmp(s, "east") ||
                 !g_ascii_strcasecmp(s, "right")) {
            o->type = RELATIVE;
            o->u.rel.dir = OB_DIRECTION_EAST;
        }
        else {
            o->type = ABSOLUTE;
            o->u.abs.desktop = atoi(s) - 1;
        }
        g_free(s);
    }

    if ((n = obt_parse_find_node(node, "wrap")))
        o->u.rel.wrap = obt_parse_node_bool(n);

    return o;
}

static gpointer setup_send_func(xmlNodePtr node)
{
    xmlNodePtr n;
    Options *o;

    o = setup_go_func(node);
    o->send = TRUE;
    o->follow = TRUE;

    if ((n = obt_parse_find_node(node, "follow")))
        o->follow = obt_parse_node_bool(n);

    return o;
}

/* Always return FALSE because its not interactive */
static gboolean run_func(ObActionsData *data, gpointer options)
{
    Options *o = options;
    guint d;

    switch (o->type) {
    case LAST:
        d = screen_last_desktop;
        break;
    case CURRENT:
        d = screen_desktop;
        break;
    case ABSOLUTE:
        d = o->u.abs.desktop;
        break;
    case RELATIVE:
        d = screen_find_desktop(screen_desktop,
                                o->u.rel.dir, o->u.rel.wrap, o->u.rel.linear);
        break;
    }

    if (d < screen_num_desktops &&
        (d != screen_desktop ||
         (data->client && data->client->desktop != screen_desktop))) {
        gboolean go = TRUE;

        actions_client_move(data, TRUE);
        if (o->send && data->client && client_normal(data->client)) {
            client_set_desktop(data->client, d, o->follow, FALSE);
            go = o->follow;
        }

        if (go) {
            screen_set_desktop(d, TRUE);
            if (data->client)
                client_bring_helper_windows(data->client);
        }

        actions_client_move(data, FALSE);
    }
    return FALSE;
}
