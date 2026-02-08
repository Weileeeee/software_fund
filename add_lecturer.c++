#include <iostream>
#include <windows.h>
#include <sqlext.h>
#include <sqltypes.h>
#include <sql.h>
#include <string>

using namespace std;

// Configuration
const string DB_HOST = "127.0.0.1";
const string DB_NAME = "CampusSecurityDB";
const string DB_USER = "root";
const string DB_PASS = "Gyl2005.";

int main() {
    SQLHENV sqlEnv;
    SQLHDBC sqlConn;
    SQLHSTMT hStmt;
    SQLRETURN ret;

    // Allocate Environment Handle
    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &sqlEnv);
    SQLSetEnvAttr(sqlEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);

    // Allocate Connection Handle
    SQLAllocHandle(SQL_HANDLE_DBC, sqlEnv, &sqlConn);

    // Connect to Database
    string connStr = "Driver={MySQL ODBC 9.5 Unicode Driver};Server=" + DB_HOST + ";Database=" + DB_NAME + ";User=" + DB_USER + ";Password=" + DB_PASS + ";Option=3;";
    cout << "Connecting to database..." << endl;
    
    ret = SQLDriverConnectA(sqlConn, NULL, (SQLCHAR*)connStr.c_str(), SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);

    if (!SQL_SUCCEEDED(ret)) {
        cout << "[FATAL] DB Connection Failed" << endl;
        SQLCHAR sqlState[1024];
        SQLCHAR message[1024];
        SQLINTEGER nativeError;
        SQLSMALLINT textLength;
        SQLGetDiagRec(SQL_HANDLE_DBC, sqlConn, 1, sqlState, &nativeError, message, 1024, &textLength);
        cout << "Message: " << message << endl;
        return -1;
    }

    cout << "Connected! Adding Lecturer Account..." << endl;

    // Allocate Statement Handle
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConn, &hStmt);

    // SQL Query to Insert Lecturer
    // Columns: username, password_hash, full_name, email, role, created_at
    string query = "INSERT INTO users (username, password_hash, full_name, email, role, created_at) "
                   "VALUES ('lecturer', 'password123', 'Dr. Tan', 'lecturer@uni.edu', 'Lecturer', NOW());";

    ret = SQLExecDirectA(hStmt, (SQLCHAR*)query.c_str(), SQL_NTS);

    if (SQL_SUCCEEDED(ret)) {
        cout << "[SUCCESS] Lecturer account added successfully!" << endl;
        cout << "Username: lecturer" << endl;
        cout << "Password: password123" << endl;
    } else {
        cout << "[ERROR] Failed to add account." << endl;
        SQLCHAR sqlState[1024];
        SQLCHAR message[1024];
        SQLINTEGER nativeError;
        SQLSMALLINT textLength;
        SQLGetDiagRec(SQL_HANDLE_STMT, hStmt, 1, sqlState, &nativeError, message, 1024, &textLength);
        cout << "Message: " << message << endl;
         // Check if it's a duplicate entry error
         if (string((char*)sqlState) == "23000") {
             cout << "Note: The username 'lecturer' or email 'lecturer@uni.edu' might already exist." << endl;
         }
    }

    // Cleanup
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    SQLDisconnect(sqlConn);
    SQLFreeHandle(SQL_HANDLE_DBC, sqlConn);
    SQLFreeHandle(SQL_HANDLE_ENV, sqlEnv);

    return 0;
}
