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
        return -1;
    }

    cout << "Connected! Fixing 'evacuationlogs' table..." << endl;

    // Allocate Statement Handle
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConn, &hStmt);

    // Step 1: Drop and recreate the table with correct schema
    string dropQuery = "DROP TABLE IF EXISTS evacuationlogs;";
    ret = SQLExecDirectA(hStmt, (SQLCHAR*)dropQuery.c_str(), SQL_NTS);
    if (SQL_SUCCEEDED(ret)) {
        cout << "[SUCCESS] Dropped old evacuationlogs table." << endl;
    }

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConn, &hStmt);

    // Step 2: Create table with updated ENUM to include 'Not Safe'
    string createQuery = "CREATE TABLE evacuationlogs ("
                        "log_id INT AUTO_INCREMENT PRIMARY KEY,"
                        "user_id INT,"
                        "status ENUM('Safe', 'Missing', 'Unknown', 'Not Safe') DEFAULT 'Unknown',"
                        "last_updated TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,"
                        "FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE,"
                        "UNIQUE(user_id)"
                        ");";

    ret = SQLExecDirectA(hStmt, (SQLCHAR*)createQuery.c_str(), SQL_NTS);

    if (SQL_SUCCEEDED(ret)) {
        cout << "[SUCCESS] Created evacuationlogs table with updated schema." << endl;
    } else {
        cout << "[ERROR] Failed to create table." << endl;
        SQLCHAR sqlState[1024];
        SQLCHAR message[1024];
        SQLINTEGER nativeError;
        SQLSMALLINT textLength;
        SQLGetDiagRec(SQL_HANDLE_STMT, hStmt, 1, sqlState, &nativeError, message, 1024, &textLength);
        cout << "Message: " << message << endl;
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return -1;
    }

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConn, &hStmt);

    // Step 3: Populate with initial records for all students
    string populateQuery = "INSERT INTO evacuationlogs (user_id, status) "
                          "SELECT user_id, 'Unknown' FROM users WHERE role IN ('Student', 'student') "
                          "ON DUPLICATE KEY UPDATE status=status;";

    ret = SQLExecDirectA(hStmt, (SQLCHAR*)populateQuery.c_str(), SQL_NTS);

    if (SQL_SUCCEEDED(ret)) {
        cout << "[SUCCESS] Populated evacuationlogs with all students." << endl;
    } else {
        cout << "[ERROR] Failed to populate table." << endl;
        SQLCHAR sqlState[1024];
        SQLCHAR message[1024];
        SQLINTEGER nativeError;
        SQLSMALLINT textLength;
        SQLGetDiagRec(SQL_HANDLE_STMT, hStmt, 1, sqlState, &nativeError, message, 1024, &textLength);
        cout << "Message: " << message << endl;
    }

    // Cleanup
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    SQLDisconnect(sqlConn);
    SQLFreeHandle(SQL_HANDLE_DBC, sqlConn);
    SQLFreeHandle(SQL_HANDLE_ENV, sqlEnv);

    cout << "\n[COMPLETE] evacuationlogs table is ready!" << endl;
    return 0;
}
