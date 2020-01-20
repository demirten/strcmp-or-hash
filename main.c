#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <getopt.h>
#include "3rdparty/uthash/src/utlist.h"
#include "3rdparty/uthash/src/uthash.h"

#define TEST_METHOD_STRCMP      1
#define TEST_METHOD_UTHASH      2

#define MIN(a,b) (((a)<(b))?(a):(b))

struct strlist {
        char *word;
        struct strlist *next;
};

struct strhash {
        char *word;
        UT_hash_handle hh;
};

struct options {
        char *wordlist_file;
        int test_method;
};

static struct options options = {
                .wordlist_file  = NULL,
                .test_method    = TEST_METHOD_STRCMP
};

static struct option longopts[] = {
        { "wordlist-file",              required_argument,      NULL,   'f' },
        { "test-method",                required_argument,      NULL,   't' },
        { "help",                       no_argument,            NULL,   'h' },
        { NULL,                         0,                      NULL,    0  },
};

static struct strlist *searchlist       = NULL;
static struct strlist *strlist          = NULL;
static struct strhash *strhash          = NULL;

static const char * test_method_str (int method)
{
        switch(method) {
        case TEST_METHOD_STRCMP:
                return "strcmp";
        case TEST_METHOD_UTHASH:
                return "uthash";
        default:
                return "unknown";
        }
}

static void display_usage (const char *program)
{
        fprintf(stdout, "usage: %s [options]\n", program);
        fprintf(stdout, "  -f, --wordlist-file        : read word lists from file (default: NULL)\n");
        fprintf(stdout, "  -t, --test-method          : choose method to compare (default: %s, options: strcmp, uthash)\n", test_method_str(options.test_method));
        fprintf(stdout, "  -h. --help                 : this text\n");
}

static int parse_cmdline (int argc, char *argv[])
{
        int c;
        const char *test_method = NULL;
        while ((c = getopt_long(argc, argv, "f:t:h", longopts, NULL)) != -1) {
                switch (c) {
                        case 'f':
                                options.wordlist_file = optarg;
                                break;
                        case 't':
                                test_method = optarg;
                                break;
                        case 'h':
                                display_usage(argv[0]);
                                exit(1);
                        default:
                                fprintf(stderr, "unknown option: %d\n", optopt);
                                return -1;
                }
        }
        if (test_method) {
                if (strcmp(test_method, "strcmp") == 0) {
                        options.test_method = TEST_METHOD_STRCMP;
                } else if (strcmp(test_method, "uthash") == 0) {
                        options.test_method = TEST_METHOD_UTHASH;
                } else {
                        fprintf(stderr, "Unknown test method: %s\n", test_method);
                        return -1;
                }
        }
        if (options.wordlist_file == NULL) {
                fprintf(stderr, "wordlist file must be given\n");
                return -1;
        }
        return 0;
}

static int add_to_search_list (char *token)
{
        struct strlist *sl;

        if (token == NULL) {
                return -1;
        }

        sl = calloc(1, sizeof(struct strlist));
        sl->word = strdup(token);
        LL_PREPEND(searchlist, sl);

        return 0;
}

static int process_input_strcmp (char *token)
{
        struct strlist *sl;

        if (token == NULL) {
                return -1;
        }

        sl = calloc(1, sizeof(struct strlist));
        sl->word = strdup(token);
        LL_PREPEND(strlist, sl);

        add_to_search_list(token);

        return 0;
}

static int process_input_uthash (char *token)
{
        struct strhash *sh;

        if (token == NULL) {
                return -1;
        }

        HASH_FIND_STR(strhash, token, sh);

        if (sh != NULL) {
                /* already in hash */
                return 0;
        }

        sh = calloc(1, sizeof(struct strhash));
        sh->word = strdup(token);
        HASH_ADD_KEYPTR(hh, strhash, sh->word, strlen(sh->word), sh);

        add_to_search_list(token);

        return 0;
}

static int process_input (char *token)
{
        switch (options.test_method) {
        case TEST_METHOD_STRCMP:
                return process_input_strcmp(token);
        case TEST_METHOD_UTHASH:
                return process_input_uthash(token);
        default:
                return -1;
        }
}

static char * search (char *token)
{
        struct strlist *sl;
        struct strhash *sh;
        char *token_found = NULL;

        switch(options.test_method) {
        case TEST_METHOD_STRCMP:
                LL_FOREACH(strlist, sl) {
                        if (strcmp(sl->word, token) == 0) {
                                token_found = sl->word;
                                break;
                        }
                }
                break;
        case TEST_METHOD_UTHASH:
                HASH_FIND_STR(strhash, token, sh);
                if (sh) {
                        token_found = sh->word;
                }
                break;
        default:
                break;
        }

        return token_found;
}

struct timespec timespec_diff (struct timespec before, struct timespec after)
{
        struct timespec res;
        if (after.tv_sec >= before.tv_sec) {
                if ((after.tv_nsec - before.tv_nsec) < 0) {
                        res.tv_sec = after.tv_sec - before.tv_sec - 1;
                        res.tv_nsec = 1000000000 + after.tv_nsec - before.tv_nsec;
                } else {
                        res.tv_sec = after.tv_sec - before.tv_sec;
                        res.tv_nsec = after.tv_nsec - before.tv_nsec;
                }
        } else {
                if ((before.tv_nsec - after.tv_nsec) < 0) {
                        res.tv_sec = before.tv_sec - after.tv_sec - 1;
                        res.tv_nsec = 1000000000 + before.tv_nsec - after.tv_nsec;
                } else {
                        res.tv_sec = before.tv_sec - after.tv_sec;
                        res.tv_nsec = before.tv_nsec - after.tv_nsec;
                }
        }
        return res;
}

int main (int argc, char **argv)
{
        FILE *fp;
        char buffer[16*1024];
        int offset;
        size_t read;
        char *p;
        char *token;
        struct timespec before;
        struct timespec after;
        struct timespec diff;
        struct strlist *sl;
        int words_total = 0;
        int words_completed = 0;
        int percent_step;

        if (parse_cmdline(argc, argv) < 0) {
                display_usage(argv[0]);
                return 1;
        }

        if ((fp = fopen(options.wordlist_file, "r")) == NULL) {
                fprintf(stderr, "couldn't open file: %s [%s]\n", options.wordlist_file, strerror(errno));
                return 1;
        }

        offset = 0;
        read = 0;

        while (!feof(fp)) {
                size_t rc = fread(buffer + offset, sizeof(char), sizeof(buffer) - offset - 1, fp);
                p = buffer;
                while ((token = strsep(&p, "\n"))) {
                        if (process_input(token) == 0) {
                                words_total++;
                        }
                        if (strstr(p, "\n") == NULL) {
                                offset = strlen(p);
                                sprintf(buffer, "%s", p);
                                break;
                        }
                }
                if (rc > 0) {
                        read += rc;
                } else if (rc == 0) {
                        break;
                } else {
                        fprintf(stderr, "couldn't read from file: %s\n", strerror(errno));
                        return 1;
                }
        }

        fclose(fp);

        percent_step = words_total / 20;
        clock_gettime(CLOCK_MONOTONIC, &before);
        LL_FOREACH(searchlist, sl) {
                char *token = search(sl->word);
                if (token == NULL) {
                        fprintf(stderr, "this shouldn't happen, %s not found in list\n", sl->word);
                        return 1;
                }
                if (++words_completed % percent_step == 0) {
                        fprintf(stdout, "\rCompleted: %%%.0f", 100.0 * words_completed / words_total);
                        fflush(stdout);
                }
        }
        clock_gettime(CLOCK_MONOTONIC, &after);

        diff = timespec_diff(before, after);
        fprintf(stdout, "\nTotal Time: %li.%09li seconds\n", diff.tv_sec, diff.tv_nsec);

        return 0;

}
