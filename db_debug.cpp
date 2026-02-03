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
    SQLHENV sqlEnv;
    SQLHDBC sqlConn;
    SQLHSTMT sqlStmt;
    SQLRETURN ret;

    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &sqlEnv);
    SQLSetEnvAttr(sqlEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
    SQLAllocHandle(SQL_HANDLE_DBC, sqlEnv, &sqlConn);

    string connStr = "Driver={MySQL ODBC 9.5 Unicode Driver};Server=" + DB_HOST + ";Database=" + DB_NAME + ";User=" + DB_USER + ";Password=" + DB_PASS + ";Option=3;";
    
    cout << "Connecting to DB..." << endl;
    ret = SQLDriverConnectA(sqlConn, NULL, (SQLCHAR*)connStr.c_str(), SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);

    if (SQL_SUCCEEDED(ret)) {
        cout << "Connected!" << endl;
        
        SQLAllocHandle(SQL_HANDLE_STMT, sqlConn, &sqlStmt);
        
        // 1. Check Use Count
        SQLExecDirectA(sqlStmt, (SQLCHAR*)"SELECT COUNT(*) FROM Users;", SQL_NTS);
        long count = 0;
        if (SQLFetch(sqlStmt) == SQL_SUCCESS) {
            SQLGetData(sqlStmt, 1, SQL_C_LONG, &count, 0, NULL);
            cout << "Total Users: " << count << endl;
        }
        SQLFreeStmt(sqlStmt, SQL_CLOSE);

    // 2. Dump Users
    cout << "\n--- User List (ID | Name | Role | Block) ---\n";
    SQLExecDirectA(sqlStmt, (SQLCHAR*)"SELECT user_id, username, role, IFNULL(block, 'NULL'), IFNULL(phone_number, 'NULL') FROM Users;", SQL_NTS);
    
    char id[20], name[100], role[50], block[50], phone[50];
    while (SQLFetch(sqlStmt) == SQL_SUCCESS) {
        SQLGetData(sqlStmt, 1, SQL_C_CHAR, id, sizeof(id), NULL);
        SQLGetData(sqlStmt, 2, SQL_C_CHAR, name, sizeof(name), NULL);
        SQLGetData(sqlStmt, 3, SQL_C_CHAR, role, sizeof(role), NULL);
        SQLGetData(sqlStmt, 4, SQL_C_CHAR, block, sizeof(block), NULL);
        SQLGetData(sqlStmt, 5, SQL_C_CHAR, phone, sizeof(phone), NULL);
        
        cout << id << " | " << name << " | " << role << " | " << block << " | " << phone << endl;
    }
    cout << "------------------------------------------\n";

        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmt);
        SQLDisconnect(sqlConn);
    } else {
        cout << "Connection Failed." << endl;
    }

    SQLFreeHandle(SQL_HANDLE_DBC, sqlConn);
    SQLFreeHandle(SQL_HANDLE_ENV, sqlEnv);
    return 0;
}
