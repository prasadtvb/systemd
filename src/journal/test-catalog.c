/*-*- Mode: C; c-basic-offset: 8; indent-tabs-mode: nil -*-*/

/***
  This file is part of systemd.

  Copyright 2012 Lennart Poettering
  Copyright 2013 Zbigniew Jędrzejewski-Szmek

  systemd is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation; either version 2.1 of the License, or
  (at your option) any later version.

  systemd is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with systemd; If not, see <http://www.gnu.org/licenses/>.
***/

#include <locale.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "util.h"
#include "log.h"
#include "macro.h"
#include "sd-messages.h"
#include "catalog.h"

static const char *catalog_dirs[] = {
        CATALOG_DIR,
        NULL,
};

static const char *no_catalog_dirs[] = {
        "/bin/hopefully/with/no/catalog",
        NULL
};

static void test_import(Hashmap *h, struct strbuf *sb,
                        const char* contents, ssize_t size, int code) {
        int r;
        char name[] = "/tmp/test-catalog.XXXXXX";
        _cleanup_close_ int fd = mkstemp(name);
        assert(fd >= 0);
        assert_se(write(fd, contents, size) == size);

        r = catalog_import_file(h, sb, name);
        assert(r == code);

        unlink(name);
}

static void test_catalog_importing(void) {
        Hashmap *h;
        struct strbuf *sb;

        assert_se(h = hashmap_new(catalog_hash_func, catalog_compare_func));
        assert_se(sb = strbuf_new());

#define BUF "xxx"
        test_import(h, sb, BUF, sizeof(BUF), -EINVAL);
#undef BUF
        assert(hashmap_isempty(h));
        log_debug("----------------------------------------");

#define BUF \
"-- 0027229ca0644181a76c4e92458afaff dededededededededededededededede\n" \
"Subject: message\n" \
"\n" \
"payload\n"
        test_import(h, sb, BUF, sizeof(BUF), -EINVAL);
#undef BUF

        log_debug("----------------------------------------");

#define BUF \
"-- 0027229ca0644181a76c4e92458afaff dededededededededededededededed\n" \
"Subject: message\n" \
"\n" \
"payload\n"
        test_import(h, sb, BUF, sizeof(BUF), 0);
#undef BUF

        assert(hashmap_size(h) == 1);

        log_debug("----------------------------------------");

        hashmap_free_free(h);
        strbuf_cleanup(sb);
}


static const char* database = NULL;

static void test_catalog_update(void) {
        int r;

        static char name[] = "/tmp/test-catalog.XXXXXX";
        r = mkstemp(name);
        assert(r >= 0);

        database = name;

        /* Test what happens if there are no files. */
        r = catalog_update(database, NULL, NULL);
        assert(r >= 0);

        /* Test what happens if there are no files in the directory. */
        r = catalog_update(database, NULL, no_catalog_dirs);
        assert(r >= 0);

        /* Make sure that we at least have some files loaded or the
           catalog_list below will fail. */
        r = catalog_update(database, NULL, catalog_dirs);
        assert(r >= 0);
}

int main(int argc, char *argv[]) {
        _cleanup_free_ char *text = NULL;
        int r;

        setlocale(LC_ALL, "de_DE.UTF-8");

        log_set_max_level(LOG_DEBUG);

        test_catalog_importing();

        test_catalog_update();

        r = catalog_list(stdout, database, true);
        assert_se(r >= 0);

        r = catalog_list(stdout, database, false);
        assert_se(r >= 0);

        assert_se(catalog_get(database, SD_MESSAGE_COREDUMP, &text) >= 0);
        printf(">>>%s<<<\n", text);

        if (database)
                unlink(database);

        return 0;
}
