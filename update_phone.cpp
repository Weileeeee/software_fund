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

const string TARGET_PHONE = "60136491330";

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

    // 3. Update Multiple Phone Numbers
    
    // Helper lambda for execution
    auto RunQuery = [&](string q, string user) {
        SQLHSTMT hStmtLocal;
        SQLAllocHandle(SQL_HANDLE_STMT, sqlConn, &hStmtLocal);
        
        if (SQL_SUCCEEDED(SQLExecDirectA(hStmtLocal, (SQLCHAR*)q.c_str(), SQL_NTS))) {
            cout << "[SUCCESS] Updated '" << user << "'." << endl;
        } else {
             SQLCHAR sqlState[10]; SQLCHAR msg[100]; SQLINTEGER nativeError; SQLSMALLINT textLen;
             SQLGetDiagRecA(SQL_HANDLE_STMT, hStmtLocal, 1, sqlState, &nativeError, msg, sizeof(msg), &textLen);
             cout << "[ERROR] Failed to update '" << user << "': " << msg << endl;
        }
        SQLFreeHandle(SQL_HANDLE_STMT, hStmtLocal);
    };

    // Update yihern
    string phoneYihern = "60182991143";
    cout << "Updating 'yihern' to: " << phoneYihern << endl;
    string query1 = "UPDATE Users SET phone_number = '" + phoneYihern + "', whatsapp_number = '" + phoneYihern + "' WHERE username = 'yihern';";
    RunQuery(query1, "yihern");

    // Update yulun_student02
    string phoneYulun02 = "60174664009";
    cout << "Updating 'yulun_student02' to: " << phoneYulun02 << endl;
    string query2 = "UPDATE Users SET phone_number = '" + phoneYulun02 + "', whatsapp_number = '" + phoneYulun02 + "' WHERE username = 'yulun_student02';";
    RunQuery(query2, "yulun_student02");

    // Cleanup
    SQLDisconnect(sqlConn);
    SQLFreeHandle(SQL_HANDLE_DBC, sqlConn);
    SQLFreeHandle(SQL_HANDLE_ENV, sqlEnv);

    cout << "\nDone." << endl;
    // cin.get(); removed to avoid process locking

    return 0;
}
