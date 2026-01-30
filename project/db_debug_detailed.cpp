#include <iostream>
#include <windows.h> 
#include <sqlext.h>
#include <sqltypes.h>
#include <sql.h>
#include <string>

using namespace std;

const string DB_HOST = "127.0.0.1";
const string DB_NAME = "CampusSecurityDB";
const string DB_USER = "root";
const string DB_PASS = "Gyl2005.";

int main() {
    SQLHENV sqlEnv; SQLHDBC sqlConn; SQLHSTMT sqlStmt;
    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &sqlEnv);
    SQLSetEnvAttr(sqlEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
    SQLAllocHandle(SQL_HANDLE_DBC, sqlEnv, &sqlConn);
    string connStr = "Driver={MySQL ODBC 9.5 Unicode Driver};Server=" + DB_HOST + ";Database=" + DB_NAME + ";User=" + DB_USER + ";Password=" + DB_PASS + ";Option=3;";
    if (SQL_SUCCEEDED(SQLDriverConnectA(sqlConn, NULL, (SQLCHAR*)connStr.c_str(), SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT))) {
        cout << "Connected!\n";
        SQLAllocHandle(SQL_HANDLE_STMT, sqlConn, &sqlStmt);
        
        // Execute the force update manually if needed, or just check values carefully
        cout << "--- Checking Roles ---\n";
        SQLExecDirectA(sqlStmt, (SQLCHAR*)"SELECT user_id, username, role, LENGTH(role) FROM Users;", SQL_NTS);
        
        char id[20], name[100], role[50];
        long len;
        while (SQLFetch(sqlStmt) == SQL_SUCCESS) {
            SQLGetData(sqlStmt, 1, SQL_C_CHAR, id, sizeof(id), NULL);
            SQLGetData(sqlStmt, 2, SQL_C_CHAR, name, sizeof(name), NULL);
            SQLGetData(sqlStmt, 3, SQL_C_CHAR, role, sizeof(role), NULL);
            SQLGetData(sqlStmt, 4, SQL_C_LONG, &len, 0, NULL);
            cout << "ID: " << id << " | Name: " << name << " | Role: '" << role << "' (Len: " << len << ")\n";
        }
        SQLDisconnect(sqlConn);
    }
    return 0;
}
