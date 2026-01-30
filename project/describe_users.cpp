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
    SQLHENV sqlEnv; SQLHDBC sqlConn; SQLHSTMT hStmt;
    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &sqlEnv);
    SQLSetEnvAttr(sqlEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
    SQLAllocHandle(SQL_HANDLE_DBC, sqlEnv, &sqlConn);
    string connStr = "Driver={MySQL ODBC 9.5 Unicode Driver};Server=" + DB_HOST + ";Database=" + DB_NAME + ";User=" + DB_USER + ";Password=" + DB_PASS + ";Option=3;";
    
    if (SQL_SUCCEEDED(SQLDriverConnectA(sqlConn, NULL, (SQLCHAR*)connStr.c_str(), SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT))) {
        cout << "Connected!\n--- Users Table Columns ---\n";
        SQLAllocHandle(SQL_HANDLE_STMT, sqlConn, &hStmt);
        SQLExecDirectA(hStmt, (SQLCHAR*)"DESCRIBE Users;", SQL_NTS);
        
        char field[100], type[100];
        while (SQLFetch(hStmt) == SQL_SUCCESS) {
            SQLGetData(hStmt, 1, SQL_C_CHAR, field, sizeof(field), NULL);
            SQLGetData(hStmt, 2, SQL_C_CHAR, type, sizeof(type), NULL);
            cout << field << " (" << type << ")" << endl;
        }
        SQLDisconnect(sqlConn);
    }
    return 0;
}
