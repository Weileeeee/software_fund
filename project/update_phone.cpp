#include <iostream>
#include <string>
#include <windows.h>
#include <sqlext.h>
#include <sqltypes.h>
#include <sql.h>

#pragma comment(lib, "odbc32.lib")

using namespace std;

// Database Config
const string DB_HOST = "127.0.0.1";
const string DB_NAME = "CampusSecurityDB";
const string DB_USER = "root";
const string DB_PASS = "Gyl2005."; 

const string TARGET_PHONE = "601110867951";

int main() {
    SQLHENV sqlEnv;
    SQLHDBC sqlConn;
    SQLHSTMT hStmt;

    // 1. Initialize ODBC
    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &sqlEnv);
    SQLSetEnvAttr(sqlEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
    SQLAllocHandle(SQL_HANDLE_DBC, sqlEnv, &sqlConn);

    // 2. Connect
    string connStr = "Driver={MySQL ODBC 9.5 Unicode Driver};Server=" + DB_HOST + ";Database=" + DB_NAME + ";User=" + DB_USER + ";Password=" + DB_PASS + ";Option=3;";
    cout << "Connecting to database..." << endl;

    if (!SQL_SUCCEEDED(SQLDriverConnectA(sqlConn, NULL, (SQLCHAR*)connStr.c_str(), SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT))) {
        cout << "[FATAL] DB Connection Failed" << endl;
        return -1;
    }

    SQLAllocHandle(SQL_HANDLE_STMT, sqlConn, &hStmt);

    // 3. Update Phone Number
    // Updating ALL students mainly for testing purposes, or specific user if needed.
    // The user asked "change student phone number", assuming for the test account.
    cout << "Updating phone number for all 'Student' accounts to: " << TARGET_PHONE << endl;

    string query = "UPDATE Users SET phone_number = '" + TARGET_PHONE + "', whatsapp_number = '" + TARGET_PHONE + "' WHERE role IN ('Student', 'student');";
    
    if (SQL_SUCCEEDED(SQLExecDirectA(hStmt, (SQLCHAR*)query.c_str(), SQL_NTS))) {
        SQLLEN rowCount = 0;
        SQLRowCount(hStmt, &rowCount);
        cout << "SUCCESS: Updated " << rowCount << " student records." << endl;
    } else {
        cout << "ERROR: Failed to update records." << endl;
    }

    // Cleanup
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    SQLDisconnect(sqlConn);
    SQLFreeHandle(SQL_HANDLE_DBC, sqlConn);
    SQLFreeHandle(SQL_HANDLE_ENV, sqlEnv);

    cout << "\nDone." << endl;
    // cin.get(); removed to avoid process locking

    return 0;
}
