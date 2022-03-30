#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#include <stdio.h>

static long utc_diff = -1;

static long calc_utc_diff(void) {;
    time_t tx = time(NULL);
    struct tm lt = {0};

    localtime_r(&tx, &lt);
    //printf("Offset to GMT is %lds.\n", lt.tm_gmtoff);
    //printf("The time zone is '%s'.\n", lt.tm_zone);

    return lt.tm_gmtoff;
}

//=====================================================

static char err_msg[512];

static int  err(char *fmt, ...) {
    
    va_list args;

    va_start(args, fmt);
    if (fmt) {
        vsprintf(err_msg, fmt, args);
    }
    va_end(args);
    return -1;
}


//=====================================================
//   string_to_epoch
//=====================================================


/**
 * 
 * 
 * 
 */ 
char *string_to_epoch_err(void) {
    return err_msg;
}

/**
 * 
 * 
 * 
 */ 
unsigned long string_to_epoch(char *str) {
    char *s = str;
    char buf[30];

    // pula brancos iniciais
    while (*s == ' ') s++;

    int n = sizeof(buf) - 1;
    char *d = buf;
    while (*s && n) {
        if (*s == '.') break;
        *d = *s;
        s++;
        d++;
        n--;
    }
    *d = '\0';
    if (!*buf) return 0;
    return  ( (unsigned long)  atoll(buf)) + 1;
}


//=====================================================
//   datetime_to_epoch
//=====================================================

#define YY 0
#define MM 1
#define DD 2
#define HH 3
#define NN 4
#define SS 5

/**
 * 
 * 
 * 
 */ 
char *datetime_to_epoch_err(void) {
    return err_msg;
}

/**
 * 
 * 
 * 
 */ 
long datetime_to_epoch(char *fmt, char *str, char *what) {
    int map_char[26] = {
         0, 0, 0, 3, 0, 0, 0, 4, 0, 0, // a b c d e f g h i j
         0, 0, 2, 5, 0, 0, 0, 0, 6, 7, // k l m n o p q r s t
         0, 0, 0, 0, 1, 0              // u v w x y z
    };

    char *f = fmt;
    char *d = str;

    int  v[7] = { -1, -1, -1, 0, 0, 0, 0 };
    char buf[10];

    // pula brancos iniciais
    while (*d == ' ') d++;

    int idx = 0, idx0 = 0;
    char *s = buf;
    int c = 0;  

    for ( ; *f && *d; f++, d++ ) {
        // accepts / denies special characters 
        if ( ( (*f == *d) && ((*f == '/') || (*f == ':')  || (*f == ' ') || (*f == '-')) )  || (*f == 'T')) {
            if (idx0) {
                *s = '\0'; 
                v[idx0-1] = atoi(buf); 
                idx0 = 0;
            }
            s = buf;
            c = 0;
            continue;
        }

        // handle numbers
        int pos = *f - 'a';

        // map letters to indexes 
        if ((pos < 0) || (pos >= 26) || !map_char[pos])  {
            return err("Invalid date_to_epoch format: '%s' <- %s (%c)",fmt, str,*f);
        }
        idx = map_char[pos];

        if (idx0 != idx) { 
            if (idx0) {
                *s = '\0'; 
                v[idx0-1] = atoi(buf); 
            }
            s = buf; 
            idx0 = idx;
            c = 0;
        }

        if (c > 5) {
            return err("Invalid date_to_epoch format: '%s' <- %s",fmt, str);
        }

        *s = *d;
        s++; 
        c++;
       
    }

    // Did the number finish before format? 
    if (*f) {
        return err("Invalid date_to_epoch value: '%s'",str);
    }

    // Does exist something in buf?
    if (idx0) {
        *s = '\0'; 
        v[idx0-1] = atoi(buf); 
    }

    int extra_hour = 0;

    // valida e ajusta os campos
    if ((v[YY] < 0) || (v[YY] > 2199)) {
        return err("Invalid year: %s", str);
    }
    if ((v[MM] < 1) || (v[MM] > 12)) {
        return err("Invalid month: %s", str);
    }
    if ((v[DD] < 1) || (v[DD] > 31)) {
        return err("Invalid day: %s", str);
    }
    if ((v[HH] < 0)) {
        return err("Invalid hour: %s", str);    
    }
    if (v[HH] > 23) {
        while (v[HH] > 23) {
            extra_hour ++;
            v[HH] -= 24;
        }
    }
    if ((v[NN] < 0) || (v[NN] > 59)) {
        return err("Invalid minute: %s", str);
    }
    if ((v[SS] < 0) || (v[SS] > 59)) {
        return err("Invalid second: %s", str);
    }

    if (v[YY] < 100) v[YY] += 2000;

#if 0
    for (int i = 0; i<6; i++) {
        printf("[%d] -> %d\n",i, v[i]);
    }
#endif
    struct tm t;
    t.tm_year  = v[YY] - 1900;
    t.tm_mon = v[MM]-1;
    t.tm_mday  = v[DD];
    t.tm_hour  = v[HH];
    t.tm_min   = v[NN];
    t.tm_sec   = v[SS];
    t.tm_isdst = 0;

    time_t tm = mktime(&t);

    if (utc_diff < 0) utc_diff = calc_utc_diff();
    unsigned long tm_utc =  (unsigned long) tm + utc_diff; // sem GMT
    // printf("%ld %lu %d %d %d %d\n",tm, tm_utc, t.tm_year, t.tm_hour, t.tm_min, t.tm_sec);
    tm_utc += extra_hour * 3600;

    if (!what) return tm_utc;
    char w = what[0];
    if (w == 'e') return tm_utc;

#define EPOCH_DOW 4
#define SECS_PER_DAY 86400 
unsigned long dow = ((tm_utc / SECS_PER_DAY) + EPOCH_DOW) % 7;    

    if (w == 'w') return dow;

#define SECONDS_PER_HOUR  3600
unsigned long hour = ((tm_utc / SECONDS_PER_HOUR) ) % 24;    
    if (w == 'h') return hour;

#define SECONDS_PER_HOUR  3600
unsigned long hours = ((tm_utc / SECONDS_PER_HOUR) );    
    if (w == 'H') {
        //static FILE *flog=NULL;
        //if (!flog) flog = fopen("datetime.log","wt");
        // printf("[%02ld]",hours);
        //fprintf(flog,"%ld\n",hours);
        //fflush(flog);
       return hours;
    }

#define SECONDS_PER_MINUTE  3600
unsigned long minutes = ((tm_utc / SECONDS_PER_MINUTE) );    
    if (w == 'M') return minutes;

    return err("Invalid what: %s", what);
}

#if 0

void main(void) {
    time_t tm = datetime_to_epoch("mm/dd/yyyy hh:nn","02/04/2001 95:07");
    if (!tm) {
        printf("Error: %s\n",datetime_to_epoch_err());
    } else {
        printf("From Unix %s\n",ctime(&tm));
    }
}

#endif
#if 0

void main(void) {
    time_t tm = string_to_epoch("1561949895.680");
    if (!tm) {
        printf("Error: %s\n",datetime_to_epoch_err());
    } else {
        printf("From Unix %s\n",ctime(&tm));
    }
    printf("Str: %s",ctime(&tm));
}

#endif

