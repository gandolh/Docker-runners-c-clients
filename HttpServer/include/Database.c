#include <sqlite3.h>
#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include "Logger.h"

/**
 * Users in db: Horia 1234, Cristian 4321, Thomas 5678
 *
 */

static char *db_path = "/mnt/c/projects/my-projects/PCD-CodeRunner/db";

sqlite3 *get_db()
{
    sqlite3 *db;
    char db_file_path[1024];
    snprintf(db_file_path, sizeof(db_file_path), "%s/codeRunning.db", db_path);
    int rc = sqlite3_open(db_file_path, &db);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);

        return NULL;
    }

    write_log("Successfully connected to the database.\n");
    return db;
}

void initialSeed()
{
    sqlite3 *db = get_db();
    char *err_msg = 0;

    char *sql = "CREATE TABLE User(Username TEXT PRIMARY KEY, Password TEXT, LastRequestTime TEXT, IsActive INTEGER);";

    int rc = sqlite3_exec(db, sql, 0, 0, &err_msg);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
    }
    else
    {
        write_log("Table User created successfully\n");
    }

    char *sql_create_code_submissions = "CREATE TABLE CodeSubmissions (Username TEXT, Code TEXT, Output TEXT, Error TEXT);";

    rc = sqlite3_exec(db, sql_create_code_submissions, 0, 0, &err_msg);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Failed to create table CodeSubmissions: %s\n", err_msg);
        sqlite3_free(err_msg);
    }
    else
    {
        write_log("Table CodeSubmissions created successfully\n");
    }

    char *sql_insert = "INSERT INTO User (Username, Password, LastRequestTime, IsActive) VALUES ('Horia', '1234', '', 0), ('Cristian', '4321', '', 0), ('Thomas', '5678', '', 0), ('admin', 'admin', '', 0);";

    rc = sqlite3_exec(db, sql_insert, 0, 0, &err_msg);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Failed to insert record %s\n", err_msg);
        sqlite3_free(err_msg);
    }
    else
    {
        write_log("Records created successfully\n");
    }
    sqlite3_close(db);
}

int updateActiveUser(const char *username)
{
    sqlite3 *db = get_db();
    char *err_msg = 0;
    char sql[256];
    int rc;

    snprintf(sql, sizeof(sql), "UPDATE User SET LastRequestTime = datetime('now'), IsActive = 1 WHERE Username='%s'", username);
    write_log("SQL: %s\n", sql);
    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Failed to update record %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 0;
    }
    else
    {
        write_log("Record updated successfully\n");
        sqlite3_close(db);
        return 1;
    }
}

void insertCodeSubmission(char *const username, char const *code, char const *output, char const *error)
{
    sqlite3 *db = get_db();
    char *err_msg = 0;
    char *sql;
    int rc;

    char *escaped_code = sqlite3_mprintf("%q", code);
    char *escaped_output = sqlite3_mprintf("%q", output);
    char *escaped_error = sqlite3_mprintf("%q", error);

    sql = sqlite3_mprintf("INSERT INTO CodeSubmissions (Username, Code, Output, Error) VALUES ('%q', '%q', '%q', '%q');", username, escaped_code, escaped_output, escaped_error);

    write_log("SQL: %s\n", sql);
    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Failed to insert record %s\n", err_msg);
        sqlite3_free(err_msg);
    }
    else
    {
        write_log("Record inserted successfully\n");
    }

    sqlite3_free(escaped_code);
    sqlite3_free(escaped_output);
    sqlite3_free(escaped_error);
    sqlite3_free(sql);
    sqlite3_close(db);
}

int logOut(const char *username)
{
    sqlite3 *db = get_db();
    char *err_msg = 0;
    char sql[256];
    int rc;

    snprintf(sql, sizeof(sql), "UPDATE User SET IsActive = 0 WHERE Username='%s'", username);
    write_log("SQL: %s\n", sql);
    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Failed to update record %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 0;
    }
    else
    {
        write_log("Record updated successfully\n");
        sqlite3_close(db);
        return 1;
    }
}

// returns 0 if fail, returns 1 if success
int handleLogin(const char *username, const char *password)
{
    sqlite3 *db = get_db();
    sqlite3_stmt *stmt;
    char sql[256];
    int rc;

    snprintf(sql, sizeof(sql), "SELECT * FROM User WHERE username='%s' AND password='%s'", username, password);
    write_log("SQL: %s\n", sql);
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        sqlite3_close(db);
        return 0;
    }

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
    {
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 1;
    }
    else
    {
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 0;
    }
}

int rm_db()
{
    char db_file_path[1024];
    snprintf(db_file_path, sizeof(db_file_path), "%s/codeRunning.db", db_path);
    return remove(db_file_path);
}

int init_db()
{
    struct stat buffer;

    char *err_msg = 0;
    char db_file_path[1024];
    snprintf(db_file_path, sizeof(db_file_path), "%s/codeRunning.db", db_path);
    int file_exists = stat(db_file_path, &buffer) == 0;
    sqlite3 *db = get_db();

    // If the file was newly created, call initialSeed
    if (!file_exists)
    {
        initialSeed(db);
    }
    sqlite3_close(db);
    return 0;
}