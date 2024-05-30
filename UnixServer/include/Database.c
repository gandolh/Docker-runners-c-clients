#include <sqlite3.h>
#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include "Logger.h"
#include <string.h>
#include <stdlib.h>
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

static int callback(void *data, int argc, char **argv, char **azColName)
{
    int i;
    char *result = (char *)data;
    for (i = 0; i < argc; i++)
    {
        strcat(result, azColName[i]);
        strcat(result, " = ");
        if (argv[i] && argv[i][0] != '\0')
        {
            strcat(result, argv[i]);
        }
        else
        {
            strcat(result, "None");
        }
        if (i < argc - 1)
        {
            strcat(result, ", ");
        }
    }
    strcat(result, "\n");
    return 0;
}

char *gettotalusers(int limit, int buffer_size)
{
    sqlite3 *db = get_db();
    char *zErrMsg = 0;
    char sql[256];                    // buffer for SQL query
    char *data = malloc(buffer_size); // allocate memory for the result string
    data[0] = '\0';                   // initialize the string

    sprintf(sql, "SELECT * FROM User LIMIT %d;", limit);
    int rc = sqlite3_exec(db, sql, callback, (void *)data, &zErrMsg);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else
    {
        fprintf(stdout, "Operation done successfully\n");
    }
    return data;
}

char *getconnectedusers(int limit, int buffer_size)
{
    sqlite3 *db = get_db();
    char *zErrMsg = 0;
    char sql[256];                    // buffer for SQL query
    char *data = malloc(buffer_size); // allocate memory for the result string
    data[0] = '\0';                   // initialize the string

    sprintf(sql, "SELECT * FROM User WHERE IsActive = 1 LIMIT %d;", limit);
    int rc = sqlite3_exec(db, sql, callback, (void *)data, &zErrMsg);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else
    {
        fprintf(stdout, "Operation done successfully\n");
    }
    return data;
}

static int callbackCodeSubmission(void *data, int argc, char **argv, char **azColName)
{
    int i;
    char *result = (char *)data;
    for (i = 0; i < argc; i++)
    {
        strcat(result, azColName[i]);
        strcat(result, " = ");
        if (argv[i] && argv[i][0] != '\0' && argv[i][0] != ' ')
        {
            if (strcmp(azColName[i], "Code") == 0 && strlen(argv[i]) > 30)
            {
                char truncated[31];
                strncpy(truncated, argv[i], 30);
                truncated[30] = '\0';
                strcat(result, truncated);
            }
            else
            {
                strcat(result, argv[i]);
            }
        }
        else
        {
            strcat(result, "None");
        }
        if (i < argc - 1)
        {
            strcat(result, ", ");
        }
    }
    strcat(result, "\n");
    return 0;
}

char *getSubmissions(int limit, int buffer_size)
{
    sqlite3 *db = get_db();
    char *zErrMsg = 0;
    char sql[256];                    // buffer for SQL query
    char *data = malloc(buffer_size); // allocate memory for the result string
    data[0] = '\0';                   // initialize the string

    sprintf(sql, "SELECT Username, substr(Code, 1, 30), Output, Error FROM CodeSubmissions LIMIT %d;", limit);
    int rc = sqlite3_exec(db, sql, callbackCodeSubmission, (void *)data, &zErrMsg);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else
    {
        fprintf(stdout, "Operation done successfully\n");
    }
    return data;
}
