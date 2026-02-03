#include <iostream>
#include <windows.h>
#include <sqlext.h>
#include <sqltypes.h>
#include <sql.h>

using namespace std;

#define DB_HOST "127.0.0.1"
#define DB_NAME "CampusSecurityDB"
#define DB_USER "root"
#define DB_PASS "admin@1234"

int main() {
    SQLHENV sqlEnv;
    SQLHDBC sqlConn;
    SQLHSTMT sqlStmt;
    SQLRETURN ret;

    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &sqlEnv);
    SQLSetEnvAttr(sqlEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
    SQLAllocHandle(SQL_HANDLE_DBC, sqlEnv, &sqlConn);

    string connStr = "Driver={MySQL ODBC 9.5 Unicode Driver};Server=" + string(DB_HOST) + ";Database=" + string(DB_NAME) + ";User=" + string(DB_USER) + ";Password=" + string(DB_PASS) + ";Option=3;";

    cout << "Connecting to database..." << endl;
    ret = SQLDriverConnectA(sqlConn, NULL, (SQLCHAR*)connStr.c_str(), SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);

    if (SQL_SUCCEEDED(ret)) {
        cout << "Connected!" << endl;
        SQLAllocHandle(SQL_HANDLE_STMT, sqlConn, &sqlStmt);

        // 1. Add description if missing (ignore error if exists)
        cout << "Adding 'description' column..." << endl;
        SQLExecDirectA(sqlStmt, (SQLCHAR*)"ALTER TABLE Incidents ADD COLUMN description TEXT;", SQL_NTS);
        
        // 2. Add severity
        cout << "Adding 'severity' column..." << endl;
        SQLExecDirectA(sqlStmt, (SQLCHAR*)"ALTER TABLE Incidents ADD COLUMN severity ENUM('Low', 'Medium', 'High', 'Critical') DEFAULT 'Medium';", SQL_NTS);
        
        // 3. Add evidence_path
        cout << "Adding 'evidence_path' column..." << endl;
        SQLExecDirectA(sqlStmt, (SQLCHAR*)"ALTER TABLE Incidents ADD COLUMN evidence_path VARCHAR(255);", SQL_NTS);

        cout << "Schema update commands sent." << endl;
    } else {
        cout << "Failed to connect to DB." << endl;
    }

    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmt);
    SQLDisconnect(sqlConn);
    SQLFreeHandle(SQL_HANDLE_DBC, sqlConn);
    SQLFreeHandle(SQL_HANDLE_ENV, sqlEnv);
    return 0;
}
