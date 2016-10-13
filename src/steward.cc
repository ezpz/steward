#include <ncurses.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>

static int zoom_level = 0; /* TODO: make not a global */

struct hist_entry {
    long bin_value; /* the value this count represents */
    unsigned long count; /* count of items in this bin */
};

struct histo_data {
    long bin_min, bin_max; /* min/max bin values (range of x-axis) */
    unsigned long n; /* number of data points stored in this data frame */
    struct hist_entry data[256];
    /* TODO: XXX: FIXME:
     * eventually, this needs to be filled in dynamically and 
     * with a fixed width. FOr testing, we are using 0x0 - 0xff range
     * so can safely get away with this
     */
};

static void raw_exit () {
    endwin ();
    exit (1); 
}

static void destroy_win (WINDOW *win) {
    wborder (win, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
    werase (win);
    wrefresh (win);
    delwin (win);
}

/* TODO: make vararg supported */
static void show_error (const char * msg) {
    WINDOW *err_win = NULL;
    int e_width = COLS / 2, e_height = strlen (msg) / e_width + 1;
    int win_x = COLS / 4, win_y = LINES / 2 - (e_height + 2) / 2;
    int i = 0;
    char *msg_buf = NULL;
    
    int old_curs = curs_set (0);

    err_win = newwin (e_height + 2, e_width + 2, win_y, win_x);
    if (NULL == err_win) {
        /* what to do here ? */
        raw_exit ();
    }
    box (err_win, 0, 0);

    mvwprintw (err_win, 0, 3, "Error:");

    msg_buf = (char *) calloc (1, e_width + 1);
    if (NULL == msg_buf) {
        destroy_win (err_win);
        raw_exit ();
    }

    for (i = 0; i < e_height; ++i) {
        memset (msg_buf, '\0', e_width);
        strncpy (msg_buf, msg + i * e_width, e_width);
        mvwprintw (err_win, i + 1, 1, msg_buf);
    }

    free (msg_buf);

    wrefresh (err_win);
    sleep (2);
    getch ();
    destroy_win (err_win);
    if (ERR != old_curs) {
        curs_set (old_curs);
    }
    refresh ();
}

static void status_line (WINDOW *win, const char *fmt, ...) {
    int h = 0, w = 0, sz = 0;
    char msg[1024] = {0}; 
    chtype border[2] = {0};
    va_list va;

    getmaxyx (win, h, w);
    sz = w < 1024 ? w : 1024;

    va_start (va, fmt);
    vsnprintf (msg, sz, fmt, va);
    va_end (va);

    /* If it looks like there is a border, preserve that aesthetic */
    /* TODO: make this more formal - what is the magic number? */
    mvwinchnstr (win, h - 1, w - 1, border, 1);
    if (0x6a == (border[0] & 0xff)) {
        wmove (win, h - 1, 1);
        whline (win, 0, w - 2);
    }

    wattron (win, A_REVERSE | A_BOLD);
    mvwprintw (win, h - 1, 1, msg);
    wattroff (win, A_REVERSE | A_BOLD);
    wrefresh (win);
}

static int get_path (char *path, int sz) {
    struct stat sb;
    WINDOW *win = NULL;
    int w = COLS / 2, h = 1;
    int x = COLS / 4, y = LINES / 2 - 1;

    win = newwin (h + 2, w + 2, y, x);
    if (NULL == win) {
        return -1;
    }

    box (win, 0, 0);
    mvwprintw (win, 1, 1, "File: ");
    echo ();
    wrefresh (win);
    wgetnstr (win, path, sz);
    noecho ();
    destroy_win (win);
    refresh ();
     
    if (-1 == stat (path, &sb)) {
        show_error (strerror (errno));
        return -1;
    }

    if (0 == sb.st_size ) {
        show_error ("Empty File");
        return -1;
    }

    return 0;
}

/* TODO: Pass in a structure for out parameters */
static 
void draw (WINDOW *win, struct histo_data *data, int *l, int *h, int *b, int *s) {
    int stride = 0;
    unsigned long i = 0, tot = 0;
    int bin = 1;
    int win_h = 0, win_w = 0;

    getmaxyx (win, win_h, win_w);

    stride = data->n / (win_w - 2);
    if ((data->n % (win_w - 2)) > 0) { ++stride; }

    win_h -= 2; /* allow for status line */
    *s = stride;

    /* Once through to get the sizes; then to print */
    for (; i < data->n; i += stride, ++bin) {
        int j = 0, s = 0;
        for (; j < stride; ++j) {
            s += data->data[tot].count;
            ++tot;
            if (tot >= data->n) { break; }
        }
        if (0 == i) {
            *l = s;
            *h = s;
            *b = bin;
        } else {
            if (s > *h) { *h = s; *b = bin; }
            if (s < *l) { *l = s; }
        }
        if (tot >= data->n) { break; }
    }
    tot = 0;
    bin = 1;
    for (i = 0; i < data->n; i += stride, ++bin) {
        int j = 0, s = 0, k = 0;
        double d = 0.0, top = *h * 1.1;
        for (; j < stride; ++j) {
            s += data->data[tot].count;
            ++tot;
            if (tot >= data->n) { break; }
        }
        d = (double)s / top;
        k = (int)(d * win_h);
        if (bin == *b) {
            wattron (win, A_REVERSE);
        }
        wmove (win, win_h - k, bin);
        wvline (win, 0, k);
        if (bin == *b) {
            wattroff (win, A_REVERSE);
        }
        if (tot >= data->n) { break; }
    }
    wrefresh (win);
}

static void histo_display (WINDOW *prev_win, struct histo_data *data) {
    WINDOW *win = NULL;
    int low = 0, high = 0, zbin = 0, stride = 0;
    int c = 0;
    int old_curs = 0;

    if (NULL == data) { return; }

    win = newwin (LINES - 5, COLS, 0, 0);
    if (NULL == win) { return; }
    old_curs = curs_set (0);
    box (win, 0, 0);
    draw (win, data, &low, &high, &zbin, &stride);

    status_line (win, 
            "Bin range: 0x%02x,0x%02x  +0x%x, -0x%x, xlim:... z:%d",
            data->bin_min, data->bin_max, high, low, zoom_level);
    
    //command_line (...);
    
    c = getch ();
    while (c != KEY_F(5)) {
        switch (c) {
            case 'z':
                --zoom_level;
                destroy_win (win);
                curs_set (old_curs);
                redrawwin (prev_win);
                wrefresh (prev_win);
                return;
            case 'Z':
                /* TODO: Resize */
                ++zoom_level;
                curs_set (old_curs);
                histo_display (win, data);
                break;
            default:
                break;
        }
        c = getch ();
    }
    --zoom_level;
    destroy_win (win);
    redrawwin (prev_win);
    wrefresh (prev_win);
    
    /* TODO: handle un-telescoping */
}

static void load_histo (struct histo_data *data) {
    char path[255] = {0};
    FILE *f = NULL;

    if (NULL == data) { 
        show_error ("Missing data in load_histo");
        return; 
    }

    if (-1 == get_path (path, 255)) { return ; }

    status_line (stdscr, "File Path: %s", path);

    f = fopen (path, "rb");
    if (NULL == f) {
        show_error (strerror (errno));
        return;
    }

    while (! feof (f) && ! ferror (f)) {
        unsigned char c = 0;
        int i = fgetc (f);
        if (EOF == i) { break; }
        c = (unsigned char) i;
        data->data[c].count++;
        if (c < data->bin_min)  { data->bin_min = c; }
        if (c > data->bin_max)  { data->bin_max = c; }
    }

    fclose (f);
    histo_display (stdscr, data);
    redrawwin (stdscr);
    refresh ();
}

static void init_histo (struct histo_data *data) {
    unsigned long i = 0;
    if (NULL == data) { return; }
    /* XXX: */ data->n = 256;
    for (; i < data->n; ++i) {
        data->data[i].bin_value = (unsigned char)(i & 0xff);
        data->data[i].count = 0;
    }
    data->bin_min = 0;
    data->bin_max = 0;
}


int main () {
    int ch = 0;
    initscr ();
    raw ();
    keypad (stdscr, TRUE);
    noecho ();

    printw ("Press F5 to exit");
    refresh ();
    while ((ch = getch ()) != KEY_F(5)) {
        struct histo_data histo;
        init_histo (&histo);
        load_histo (&histo);
    }
    endwin ();
    return 0;
}

