/* See LICENSE file for copyright and license details. */

#include "fibonacci.c"

/* appearance */
static const unsigned int borderpx  = 2;        /* border pixel of windows */
static const unsigned int gappx     = 3;        /* gaps between windows */
static const unsigned int snap      = 32;       /* snap pixel */
static const int swallowfloating    = 0;        /* 1 means swallow floating windows by default */
static const unsigned int systraypinning = 0;   /* 0: sloppy systray follows selected monitor, >0: pin systray to monitor X */
static const unsigned int systrayspacing = 2;   /* systray spacing */
static const int systraypinningfailfirst = 1;   /* 1: if pinning fails, display systray on the first monitor, False: display systray on the last monitor*/
static const int showsystray        = 1;     /* 0 means no systray */
static const int showbar            = 1;        /* 0 means no bar */
static const int topbar             = 1;        /* 0 means bottom bar */
static const char *fonts[] = { "SF Pro Display:size=11:antialias=true:autohint=true" };
static const char dmenufont[]       = "Source Code Pro:size=11";
static const char col_night1[]      = "#2E3440";
static const char col_night2[]      = "#3B4252";
static const char col_night4[]      = "#4C566a";
static const char col_snow1[]       = "#D8DEE9";
static const char col_frost2[]      = "#81A1C1";
static const char col_frost3[]      = "#88C0D0";
static const char col_frost4[]      = "#5E81AC";
static const char *colors[][3]      = {
	/*                         fg         bg          border   */
	[SchemeNorm]        = { col_snow1,  col_night1,  col_night2 },
	[SchemeSel]         = { col_night2, col_frost4,  col_frost2 },
	[SchemeStatus]      = { col_snow1,  col_night1,  "#000000"  }, // Statusbar right {text,background,not used but cannot be empty}
	[SchemeTagsSel]     = { col_night2, col_frost2,  "#000000"  }, // Tagbar left selected {text,background,not used but cannot be empty}
        [SchemeTagsNorm]    = { col_snow1,  col_night1,  "#000000"  }, // Tagbar left unselected {text,background,not used but cannot be empty}
        [SchemeInfoSel]     = { col_night2, col_frost2,  "#000000"  }, // infobar middle  selected {text,background,not used but cannot be empty}
        [SchemeInfoNorm]    = { col_snow1,  col_night1,  "#000000"  }, // infobar middle  unselected {text,background,not used but cannot be empty}
};

typedef struct {
	const char *name;
	const void *cmd;
} Sp;
const char *spcmd1[] = {"st", "-n", "spterm", "-g", "144x41", NULL };
const char *spcmd2[] = {"st", "-n", "spfm", "-g", "144x41", "-e", "ranger", NULL };
static Sp scratchpads[] = {
	/* name          cmd  */
	{"spterm",      spcmd1},
	{"spranger",    spcmd2},
};
/* tagging */
static const char *tags[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9" };
static const Rule rules[] = {
	/* xprop(1):
	 *	WM_CLASS(STRING) = instance, class
	 *	WM_NAME(STRING) = title
	 */
	/* class     instance  title           tags mask  isfloating  isterminal  noswallow  monitor */
	{ "Gimp",    NULL,     NULL,           0,         1,          0,           0,        -1 },
	{ "Firefox", NULL,     NULL,           1 << 8,    0,          0,          -1,        -1 },
	{ "st",      NULL,     NULL,           0,         0,          1,          -1,        -1 },
	{ "kitty",   NULL,     NULL,           0,         0,          1,          -1,        -1 },
	{ NULL,      NULL,     "Event Tester", 0,         1,          0,           1,        -1 }, /* xev */
	{ NULL,      "spterm",   NULL,         SPTAG(0),  1,          0,           1,        -1 }, 
	{ NULL,      "spfm",     NULL,         SPTAG(1),  1,          0,           1,        -1 }, 

};

/* layout(s) */
static const float mfact     = 0.55; /* factor of master area size [0.05..0.95] */
static const int nmaster     = 1;    /* number of clients in master area */
static const int resizehints = 1;    /* 1 means respect size hints in tiled resizals */

static const Layout layouts[] = {
	/* symbol     arrange function */
	{ "[T]",      tile },    /* first entry is default */
	{ "[F]",      NULL },    /* no layout function means floating behavior */
	{ "[M]",      monocle },
	{ "[D]",      deck },
	{ "[C]",      centeredmaster },
	{ "[>C]",     centeredfloatingmaster },
 	{ "[@]",      spiral },
 	{ "[\\]",     dwindle },
};

/* key definitions */
#define MODKEY Mod4Mask
#define TAGKEYS(KEY,TAG) \
	{ MODKEY,                       KEY,      view,           {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask,           KEY,      toggleview,     {.ui = 1 << TAG} }, \
	{ MODKEY|ShiftMask,             KEY,      tag,            {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask|ShiftMask, KEY,      toggletag,      {.ui = 1 << TAG} },

/* helper for spawning shell commands in the pre dwm-5.0 fashion */
#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }

/* commands */
static char dmenumon[2] = "0"; /* component of dmenucmd, manipulated in spawn() */
static const char *dmenucmd[] = { "dmenu_run_history", "-m", dmenumon, "-fn", dmenufont, "-nb", col_night1, "-nf", col_snow1, "-sb", col_frost3, "-sf", col_night1, NULL };
static const char *termcmd[]  = {"kitty", NULL };
static const char *lockcmd[] = {"betterlockscreen","-l",NULL};
static const char *switchMonSettings[] = {"dmenu_monitor",NULL};
static const char *shutdowm[] = {"dmenu_shutdown",NULL};

static Key keys[] = {
	/* modifier                     key                    function        argument */
	{ MODKEY,                       XK_p,                  spawn,          {.v = dmenucmd } },
	{ MODKEY|ShiftMask,             XK_Return,             spawn,          {.v = termcmd } },
	{ MODKEY,                       XK_b,                  togglebar,      {0} },
	{ MODKEY,                       XK_j,                  focusstack,     {.i = +1 } },
	{ MODKEY,                       XK_k,                  focusstack,     {.i = -1 } },
	{ MODKEY|ShiftMask,             XK_j,                  pushdown,       {.i = +1 } },
	{ MODKEY|ShiftMask,             XK_k,                  pushup,         {.i = -1 } },
	{ MODKEY|ControlMask,           XK_h,                  incnmaster,     {.i = +1 } },
	{ MODKEY|ControlMask,           XK_l,                  incnmaster,     {.i = -1 } },
	{ MODKEY,                       XK_h,                  setmfact,       {.f = -0.05} },
	{ MODKEY,                       XK_l,                  setmfact,       {.f = +0.05} },
	{ MODKEY,                       XK_Return,             zoom,           {0} },
	{ MODKEY,                       XK_Tab,                view,           {0} },
	{ MODKEY|ShiftMask,             XK_q,                  killclient,     {0} },
	{ MODKEY,                       XK_t,                  setlayout,      {.v = &layouts[0]} },//"[T]"
	{ MODKEY,                       XK_f,                  setlayout,      {.v = &layouts[1]} },//"[F]"
	{ MODKEY,                       XK_m,                  setlayout,      {.v = &layouts[2]} },//"[M]"
	{ MODKEY,                       XK_d,                  setlayout,      {.v = &layouts[3]} },//"[D]"
	{ MODKEY,                       XK_c,                  setlayout,      {.v = &layouts[4]} },//"[C]"
	{ MODKEY,                       XK_s,                  setlayout,      {.v = &layouts[5]} },//"[>C]
	{ MODKEY,                       XK_x,                  setlayout,      {.v = &layouts[6]} },//"[@]"
	{ MODKEY,                       XK_z,                  setlayout,      {.v = &layouts[7]} },//"[\\]
	{ MODKEY,                       XK_space,              setlayout,      {0} },
	{ MODKEY|ShiftMask,             XK_space,              togglefloating, {0} },
	{ MODKEY,                       XK_0,                  view,           {.ui = ~0 } },
	{ MODKEY|ShiftMask,             XK_0,                  tag,            {.ui = ~0 } },
	{ MODKEY,                       XK_comma,              focusmon,       {.i = -1 } },
	{ MODKEY,                       XK_period,             focusmon,       {.i = +1 } },
	{ MODKEY|ShiftMask,             XK_comma,              tagmon,         {.i = -1 } },
	{ MODKEY|ShiftMask,             XK_period,             tagmon,         {.i = +1 } },
	{ MODKEY,                       XK_minus,              setgaps,        {.i = -1 } },
	{ MODKEY,                       XK_equal,              setgaps,        {.i = +1 } },
	{ MODKEY|ShiftMask,             XK_equal,              setgaps,        {.i = 0  } },
	{ MODKEY|ControlMask,	        XK_bracketright,       togglescratch,  {.ui = 0 } },
	{ MODKEY|ControlMask,		XK_bracketleft,        togglescratch,  {.ui = 1 } },
        { MODKEY,                       XK_Escape,             spawn,          {.v = lockcmd } },
        { MODKEY|Mod1Mask,              XK_0,                  spawn,          {.v = switchMonSettings } },
        { MODKEY|Mod1Mask,              XK_minus,              spawn,          {.v = shutdowm } },
	TAGKEYS(                        XK_1,                      0)
	TAGKEYS(                        XK_2,                      1)
	TAGKEYS(                        XK_3,                      2)
	TAGKEYS(                        XK_4,                      3)
	TAGKEYS(                        XK_5,                      4)
	TAGKEYS(                        XK_6,                      5)
	TAGKEYS(                        XK_7,                      6)
	TAGKEYS(                        XK_8,                      7)
	TAGKEYS(                        XK_9,                      8)
	{ MODKEY|ShiftMask|ControlMask, XK_q,                 quit,           {0} },
};

/* button definitions */
/* click can be ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
static Button buttons[] = {
	/* click                event mask      button          function        argument */
	{ ClkLtSymbol,          0,              Button1,        setlayout,      {0} },
	{ ClkLtSymbol,          0,              Button3,        setlayout,      {.v = &layouts[2]} },
	{ ClkWinTitle,          0,              Button2,        zoom,           {0} },
	{ ClkStatusText,        0,              Button2,        spawn,          {.v = termcmd } },
	{ ClkClientWin,         MODKEY,         Button1,        movemouse,      {0} },
	{ ClkClientWin,         MODKEY,         Button2,        togglefloating, {0} },
	{ ClkClientWin,         MODKEY,         Button3,        resizemouse,    {0} },
	{ ClkTagBar,            0,              Button1,        view,           {0} },
	{ ClkTagBar,            0,              Button3,        toggleview,     {0} },
	{ ClkTagBar,            MODKEY,         Button1,        tag,            {0} },
	{ ClkTagBar,            MODKEY,         Button3,        toggletag,      {0} },
};

