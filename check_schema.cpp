#include <iostream>
#include <windows.h>
#include <sqlext.h>
#include <sqltypes.h>
#include <sql.h>
#include <string>
#include <vector>

using namespace std;

const string DB_HOST = "127.0.0.1";
const string DB_NAME = "CampusSecurityDB";
const string DB_USER = "root";
const string DB_PASS = "admin@1234"; // From login.c++

void CheckError(SQLHANDLE handle, SQLSMALLINT type, string msg) {
    SQLINTEGER i = 0;
    SQLINTEGER native;
    SQLCHAR state[7];
    SQLCHAR text[256];
    SQLSMALLINT len;
    SQLRETURN ret;

    cout << "[ERROR] " << msg << endl;
    do {
        ret = SQLGetDiagRec(type, handle, ++i, state, &native, text, sizeof(text), &len);
        if (SQL_SUCCEEDED(ret)) {
            cout << "  State=" << state << " Native=" << native << " Msg=" << text << endl;
        }
    } while (ret == SQL_SUCCESS);
}

void DescribeTable(SQLHDBC sqlConn, string tableName) {
    SQLHSTMT hStmt;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConn, &hStmt);
    string query = "DESCRIBE " + tableName;
    cout << "\n--- DESCRIBE " << tableName << " ---\n";
    if (SQLExecDirectA(hStmt, (SQLCHAR*)query.c_str(), SQL_NTS) == SQL_SUCCESS) {
        char field[100], type[100];
        while (SQLFetch(hStmt) == SQL_SUCCESS) {
            SQLGetData(hStmt, 1, SQL_C_CHAR, field, sizeof(field), NULL);
            SQLGetData(hStmt, 2, SQL_C_CHAR, type, sizeof(type), NULL);
            cout << field << " \t " << type << endl;
        }
    } else {
        CheckError(hStmt, SQL_HANDLE_STMT, "Failed to describe " + tableName);
    }
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
}

int main() {
    SQLHENV sqlEnv; SQLHDBC sqlConn;
    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &sqlEnv);
    SQLSetEnvAttr(sqlEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
    SQLAllocHandle(SQL_HANDLE_DBC, sqlEnv, &sqlConn);
    
    // Try both passwords seen in codebase just in case
    // user: root, pass: admin@1234 (from login.c++)
    // list_tables.cpp had Gyl2005.
    
    // Try with login.c++ credentials first
    string connStr = "Driver={MySQL ODBC 9.5 Unicode Driver};Server=" + DB_HOST + ";Database=" + DB_NAME + ";User=" + DB_USER + ";Password=" + DB_PASS + ";Option=3;";
    cout << "Connecting with " << DB_PASS << "...\n";
    
    if (SQL_SUCCEEDED(SQLDriverConnectA(sqlConn, NULL, (SQLCHAR*)connStr.c_str(), SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT))) {
        cout << "Connected Success!\n";
        DescribeTable(sqlConn, "Incidents");
        DescribeTable(sqlConn, "IncidentMedia");
        SQLDisconnect(sqlConn);
    } else {
        CheckError(sqlConn, SQL_HANDLE_DBC, "Connection Failed with admin@1234");
        
        // Try Gyl2005.
        string dbPass2 = "Gyl2005.";
        string connStr2 = "Driver={MySQL ODBC 9.5 Unicode Driver};Server=" + DB_HOST + ";Database=" + DB_NAME + ";User=" + DB_USER + ";Password=" + dbPass2 + ";Option=3;";
        cout << "Retrying with " << dbPass2 << "...\n";
        if (SQL_SUCCEEDED(SQLDriverConnectA(sqlConn, NULL, (SQLCHAR*)connStr2.c_str(), SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT))) {
            cout << "Connected Success!\n";
            DescribeTable(sqlConn, "Incidents");
            DescribeTable(sqlConn, "IncidentMedia");
            SQLDisconnect(sqlConn);
        } else {
            CheckError(sqlConn, SQL_HANDLE_DBC, "Connection Failed with Gyl2005.");
        }
    }
    
    SQLFreeHandle(SQL_HANDLE_DBC, sqlConn);
    SQLFreeHandle(SQL_HANDLE_ENV, sqlEnv);
    return 0;
}
