double START;
static Eina_Bool gv_log = EINA_TRUE;
static Eina_Bool gv_log_debug = EINA_TRUE;
static Eina_Bool gv_log_info = EINA_TRUE;
#define _CR_ "%c[%d;%d;%dm"
#define _DEF_ "%c[%dm"
// TEXT MODE
#define DEF 0x1B, 0
#define CRDEF(COLOR, BACKGROUND) 0x1B, 0, COLOR, BACKGROUND
#define CRBLD(COLOR, BACKGROUND) 0x1B, 1, COLOR, BACKGROUND
// TEXT COLOR
#define CRBLK 30
#define CRRED 31
#define CRGRN 32
#define CRYLW 33
#define CRBLU 34
#define CRMGT 35
#define CRCYN 36
#define CRWHT 37
// BACKGROUND COLOR
#define BGBLK 40
#define BGRED 41
#define BGGRN 42
#define BGYLW 43
#define BGBLU 44
#define BGMGT 45
#define BGCYN 46
#define BGWHT 47

#define CRCRI CRDEF(CRWHT, BGRED)
#define CRDBG CRDEF(CRBLK, BGGRN)
#define CRINF CRDEF(CRWHT, BGBLK)

#define SHPRT_TIME printf(_CR_"[%.4f]"_DEF_"::", CRBLD(CRBLK, BGYLW), ecore_time_get() - START, DEF);
#define SHPRT_LOGO(_LOG_, _LOGCR_) printf(_CR_ _LOG_ _DEF_"::", _LOGCR_, DEF);
#define SHPRT_FUNC printf(_CR_"[%s()"_CR_":%d"_CR_"]"_DEF_"::", CRDEF(CRBLU, BGWHT), __FUNCTION__, CRDEF(CRRED, BGWHT), __LINE__, CRDEF(CRBLU, BGWHT), DEF);
#define SHPRT(_MSG_, _MSGCR_, ...) printf(_CR_ _MSG_, _MSGCR_, ## __VA_ARGS__);
#define SHPRT_ENDL printf(_DEF_"\n", DEF);

#define SHCRI(MSG, ...)  if (gv_log) { \
                           SHPRT_TIME \
                           SHPRT_LOGO("[SH-CRITICAL]", CRBLD(CRRED, BGBLK)) \
                           SHPRT_FUNC \
                           SHPRT(MSG, CRCRI, ## __VA_ARGS__) \
                           SHPRT_ENDL}
#define SHDBG(MSG, ...)  if (gv_log && gv_log_debug) { \
                           SHPRT_TIME \
                           SHPRT_LOGO("[  SH-DEBUG ]", CRBLD(CRYLW, BGMGT)) \
                           SHPRT_FUNC \
                           SHPRT(MSG, CRCRI, ## __VA_ARGS__) \
                           SHPRT_ENDL}
#define SHINF(MSG, ...) if (gv_log && gv_log_info) { \
                           SHPRT_TIME \
                           SHPRT_LOGO("[  SH-INFO  ]", CRBLD(CRGRN, BGCYN)) \
                           SHPRT_FUNC \
                           SHPRT(MSG, CRINF, ## __VA_ARGS__) \
                           SHPRT_ENDL}

#define SHNAME(_OBJ) efl_class_name_get(efl_class_get(_OBJ))
