/*
 * util.c: common, generic utility functions
 *
 * See COPYING for the license of this software
 */

#include <config.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "util.h"

int gdebug;

/*
 * Private function name:   get_datetime
 * Since version:           0.4.2
 * Description:             Function can be used to get date and time in the `YYYY-mm-dd HH:mm:ss` format, internally used for logging when debug logging is enabled using libvirt_set_logfile() API function.
 * Arguments:               None
 * Returns:                 Date/time string in `YYYY-mm-dd HH:mm:ss` format
 */
static char *
get_datetime(void)
{
    /* Caution: Function cannot use DPRINTF() macro otherwise the never ending loop will be met! */
    char *outstr = NULL;
    time_t t;
    struct tm *tmp;

    t = time(NULL);
    tmp = localtime(&t);
    if (tmp == NULL)
        return NULL;

    outstr = (char *)malloc(32 * sizeof(char));
    if (strftime(outstr, 32, "%Y-%m-%d %H:%M:%S", tmp) == 0)
        return NULL;

    return outstr;
}

void debugPrint(const char *source,
                const char *fmt, ...)
{
    char *datetime;
    va_list args;

    if (!gdebug)
        return;

    datetime = get_datetime();
    fprintf(stderr, "[%s libvirt-php/%s ]: ", datetime, source);
    VIR_FREE(datetime);

    if (fmt) {
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        va_end(args);
    }
    fprintf(stderr, "\n");
    fflush(stderr);
}

void setDebug(int level)
{
    gdebug = level;
}

int runCommand(const char *cmd,
               char **reply)
{
    char *rep = NULL;
    size_t rep_len = 0;
    FILE *fp = popen(cmd, "r");
    int ret = -1;

    if (fp == NULL)
        return -1;

    while (!feof(fp)) {
        char tmp[1024] = { 0 };
        size_t tmp_len = 0;
        char *rep_new = NULL;

        if (!fgets(tmp, sizeof(tmp) - 1, fp))
            break;

        tmp_len = strlen(tmp);

        rep_new = realloc(rep, rep_len + tmp_len + 1);
        if (!rep_new)
            goto cleanup;

        rep = rep_new;

        memcpy(rep + rep_len, tmp, tmp_len + 1);
        rep_len += tmp_len;
    }

    *reply = rep;
    rep = NULL;
    ret = 0;
 cleanup:
    fclose(fp);
    VIR_FREE(rep);
    return ret;
}
