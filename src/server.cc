#include <deque>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <sstream>
#include <string>

extern "C" {
#include <sqlite3.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <pthread.h>
#include <signal.h>
}

#define FIFO_NAME "/tmp/call.fifo"

/* TODO: Make these not global */
bool reader_running = true, writer_running = true;

void handle_sigint (int sig) {
    (void) sig;
    reader_running = false;
    writer_running = false;
};

struct call_data {
    unsigned long tv_sec, tv_usec;
    int id;
};

struct thread_data {
    std::deque<call_data> **recv_queue;
    bool *running;
    pthread_mutex_t *mutex;
    sqlite3 *db;
};

void empty (sqlite3 *db, std::deque<call_data> *calls) {

    std::stringstream ss;
    char *err_msg = NULL;
    int rc = 0;

    if (NULL == db || NULL == calls) { return; }

    fprintf (stderr, "Emptying %zd calls...\n", calls->size ());
    if (calls->empty ()) { return; }

    ss << "BEGIN TRANSACTION; ";
    while (! calls->empty ()) {
        call_data& c = calls->front ();
        ss << "INSERT INTO Test VALUES(" 
            << c.id << "," << c.tv_sec 
            << "," << c.tv_usec << "); ";
        calls->pop_front ();
    }
    ss << "COMMIT;";

    rc = sqlite3_exec(db, ss.str ().c_str (), 0, 0, &err_msg);
    if (SQLITE_OK != rc) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);        
    } 

}

void *reader_fun (void *arg) {
    thread_data *data = (thread_data *)arg;
    FILE *fp = NULL;
    /* Wait for writer thread to initialize queue */
    while (NULL == data->recv_queue) {
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 500000; 
        int err = select (0, NULL, NULL, NULL, &tv);
        if (-1 == err) {
            fprintf (stderr, "Failed to sleep: %s\n", strerror (errno));
            return NULL;
        }
    }
    int rv = mknod (FIFO_NAME, S_IFIFO|0666, 0);
    if (0 != rv && EEXIST != errno) {
        fprintf (stderr, "Failed to create FIFO node: %s\n", strerror (errno));
        return NULL;
    }

    fp = fopen (FIFO_NAME, "rb");
    if (NULL == fp) {
        fprintf (stderr, "Failed to open FIFO: %s\n", strerror (errno));
        return NULL;
    }

    while (*data->running && ! feof (fp) && ! ferror (fp)) {
        call_data cd;
        ssize_t nread = fread (&cd, sizeof (call_data), 1, fp);
        if (1 != nread) {
            continue;
        }
        pthread_mutex_lock (data->mutex);
        (*(data->recv_queue))->push_back (cd);
        pthread_mutex_unlock (data->mutex);    
    }

    fclose (fp);
    printf ("reader exiting\n");
    return NULL;
}

void *writer_fun (void *arg) {
    thread_data *data = (thread_data *)arg;
    std::deque<call_data> q1, q2, *drain;
    bool using_q1 = true;
    *(data->recv_queue) = &q1;
    while (*data->running) {
        struct timeval tv;
        int err = 0;
        tv.tv_sec = 5;
        tv.tv_usec = 500000; /* 5-1/2 second interval */
        drain = NULL;
        err = select (0, NULL, NULL, NULL, &tv);
        if (-1 == err) {
            fprintf (stderr, "Failed to sleep: %s\n", strerror (errno));
            return NULL;
        }
        err = pthread_mutex_lock (data->mutex);
        if (0 == err) {
            if (using_q1) {
                *(data->recv_queue) = &q2;
                drain = &q1;
            } else {
                *(data->recv_queue) = &q1;
                drain = &q2;
            }
            using_q1 = !using_q1;
            pthread_mutex_unlock (data->mutex);
        } else {
            fprintf (stderr, "Failed to lock mutex: %s\n", strerror (err));
            continue;
        }
        empty (data->db, drain);
    }
    printf ("writer exiting\n");
    return NULL;
}

sqlite3 *db_init (const char *db_name) {

    sqlite3 *db = NULL;
    char *err_msg = NULL;
    std::string sql;

    int rv = sqlite3_open(db_name, &db);
    if (SQLITE_OK != rv) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return NULL;
    }

    sql = "DROP TABLE IF EXISTS Test;" ;
    sql += "CREATE TABLE Test(Id INT, Sec INT, USec Int);";
    rv = sqlite3_exec(db, sql.c_str (), 0, 0, &err_msg);
    if (SQLITE_OK != rv) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);        
        sqlite3_close(db);
        return NULL;
    } 

    return db;
}

int main(void) {

    sqlite3 *db = NULL;
    int rv = 0;
    std::deque<call_data> *qp = NULL;
    thread_data reader_data, writer_data;
    pthread_t reader_thread, writer_thread;
    pthread_mutex_t mutex;

    db = db_init ("test.db");
    if (NULL == db) {
        fprintf (stderr, "Failed to initialize database\n");
        return 1;
    }

    reader_data.recv_queue = &qp;
    reader_data.running = &reader_running;
    reader_data.mutex = &mutex;
    reader_data.db = NULL;

    writer_data.recv_queue = &qp;
    writer_data.running = &writer_running;
    writer_data.mutex = &mutex;
    writer_data.db = db;

    pthread_mutex_init (&mutex, NULL);

    signal (SIGINT, handle_sigint);

    rv = pthread_create (&writer_thread, NULL, writer_fun, (void *)&writer_data);
    if (0 != rv) {
        fprintf (stderr, "Failed to create writer thread: %s\n", strerror (rv));
        return 1;
    }
    
    rv = pthread_create (&reader_thread, NULL, reader_fun, (void *)&reader_data);
    if (0 != rv) {
        fprintf (stderr, "Failed to create reader thread: %s\n", strerror (rv));
        return 1;
    }

    pthread_join (reader_thread, NULL);
    pthread_join (writer_thread, NULL);
    fprintf (stderr, "All threads exited. Closing database connection.\n");
    
    sqlite3_close(db);

    return 0;
}
