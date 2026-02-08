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

    cout << "Connected! Assigning Yu Lun to class..." << endl;

    // Allocate Statement Handle
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConn, &hStmt);

    // Step 1: Get Yu Lun's user_id
    string queryGetId = "SELECT user_id FROM users WHERE full_name = 'Yu Lun';";
    ret = SQLExecDirectA(hStmt, (SQLCHAR*)queryGetId.c_str(), SQL_NTS);
    
    int userId = 0;
    if (SQL_SUCCEEDED(ret) && SQLFetch(hStmt) == SQL_SUCCESS) {
        SQLGetData(hStmt, 1, SQL_C_LONG, &userId, sizeof(userId), NULL);
        cout << "[INFO] Found Yu Lun with user_id: " << userId << endl;
    } else {
        cout << "[ERROR] Yu Lun not found in users table!" << endl;
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return -1;
    }

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConn, &hStmt);

    // Step 2: Insert or update studentprofiles with class assignment
    string classGroup = "TCS3014"; // Assign to a sample class
    string queryAssign = "INSERT INTO studentprofiles (user_id, student_code, class_group) "
                        "VALUES (" + to_string(userId) + ", 'yulun_student', '" + classGroup + "') "
                        "ON DUPLICATE KEY UPDATE class_group = '" + classGroup + "';";

    ret = SQLExecDirectA(hStmt, (SQLCHAR*)queryAssign.c_str(), SQL_NTS);

    if (SQL_SUCCEEDED(ret)) {
        cout << "[SUCCESS] Yu Lun assigned to class: " << classGroup << endl;
    } else {
        cout << "[ERROR] Failed to assign class." << endl;
        SQLCHAR sqlState[1024];
        SQLCHAR message[1024];
        SQLINTEGER nativeError;
        SQLSMALLINT textLength;
        SQLGetDiagRec(SQL_HANDLE_STMT, hStmt, 1, sqlState, &nativeError, message, 1024, &textLength);
        cout << "SQL Error: " << message << endl;
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return -1;
    }

    // Cleanup
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    SQLDisconnect(sqlConn);
    SQLFreeHandle(SQL_HANDLE_DBC, sqlConn);
    SQLFreeHandle(SQL_HANDLE_ENV, sqlEnv);

    cout << "\n[COMPLETE] Yu Lun is now in class " << classGroup << "!" << endl;
    cout << "Refresh the lecturer page to see Yu Lun in the student list." << endl;
    return 0;
}
