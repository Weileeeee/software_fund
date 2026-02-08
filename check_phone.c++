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

    cout << "Connected! Checking Yu Lun's phone number..." << endl;

    // Allocate Statement Handle
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConn, &hStmt);

    // Check Yu Lun's phone number
    string query = "SELECT user_id, full_name, whatsapp_number FROM users WHERE full_name = 'Yu Lun';";
    ret = SQLExecDirectA(hStmt, (SQLCHAR*)query.c_str(), SQL_NTS);
    
    if (SQL_SUCCEEDED(ret)) {
        if (SQLFetch(hStmt) == SQL_SUCCESS) {
            int userId;
            char fullName[100];
            char phone[50];
            
            SQLGetData(hStmt, 1, SQL_C_LONG, &userId, sizeof(userId), NULL);
            SQLGetData(hStmt, 2, SQL_C_CHAR, fullName, sizeof(fullName), NULL);
            
            SQLLEN phoneIndicator;
            SQLGetData(hStmt, 3, SQL_C_CHAR, phone, sizeof(phone), &phoneIndicator);
            
            cout << "\n=== Yu Lun's Details ===" << endl;
            cout << "User ID: " << userId << endl;
            cout << "Full Name: " << fullName << endl;
            
            if (phoneIndicator == SQL_NULL_DATA) {
                cout << "WhatsApp Number: NULL (NOT SET!)" << endl;
                cout << "\n⚠️  PROBLEM FOUND: Yu Lun has no WhatsApp number!" << endl;
                cout << "This is why notifications are not being sent." << endl;
            } else {
                cout << "WhatsApp Number: " << phone << endl;
                cout << "\n✅ Phone number is set." << endl;
            }
        } else {
            cout << "[ERROR] Yu Lun not found in database!" << endl;
        }
    }

    // Cleanup
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    SQLDisconnect(sqlConn);
    SQLFreeHandle(SQL_HANDLE_DBC, sqlConn);
    SQLFreeHandle(SQL_HANDLE_ENV, sqlEnv);

    return 0;
}
