#ifndef DATABASE_LIB_H
#define DATABASE_LIB_H
#include "sqlite3.h"

void rm_db();
int handleLogin(char const *username, char const *password);
void updateActiveUser(char const *username);
char *gettotalusers(int limit, int buffer_size);
char *getconnectedusers(int limit, int buffer_size);
char *getSubmissions(int limit, int buffer_size);
#endif
