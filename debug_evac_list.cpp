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
        cout << "Connected to DB!\n";
        SQLAllocHandle(SQL_HANDLE_STMT, sqlConn, &hStmt);

        // EXACT QUERY FROM login.c++
        string query = "SELECT u.user_id, u.full_name, u.block, IFNULL(sp.safety_status, 'Unknown') FROM Users u LEFT JOIN studentprofiles sp ON u.user_id = sp.user_id WHERE u.role IN ('Student', 'student') ORDER BY u.block;";
        
        cout << "Executing Query: " << query << endl;

        SQLRETURN ret = SQLExecDirectA(hStmt, (SQLCHAR*)query.c_str(), SQL_NTS);
        if (!SQL_SUCCEEDED(ret)) {
             cout << "Query Execution Failed.\n";
             SQLCHAR sqlState[1024];
             SQLCHAR message[1024];
             SQLGetDiagRecA(SQL_HANDLE_STMT, hStmt, 1, sqlState, NULL, message, 1024, NULL);
             cout << "SQL Error: " << message << endl;
        } else {
            char id[20] = "", name[100] = "", blk[50] = "", stat[50] = "";
            SQLLEN indId, indName, indBlk, indStat;
            int count = 0;

            while (SQLFetch(hStmt) == SQL_SUCCESS) {
                count++;
                SQLGetData(hStmt, 1, SQL_C_CHAR, id, sizeof(id), &indId);
                SQLGetData(hStmt, 2, SQL_C_CHAR, name, sizeof(name), &indName);
                SQLGetData(hStmt, 3, SQL_C_CHAR, blk, sizeof(blk), &indBlk);
                SQLGetData(hStmt, 4, SQL_C_CHAR, stat, sizeof(stat), &indStat);

                string sBlock = (indBlk == SQL_NULL_DATA) ? "N/A" : string(blk);
                string sStatus = (indStat == SQL_NULL_DATA) ? "Unknown" : string(stat);

                cout << "Row " << count << ": " << id << " | " << name << " | " << sBlock << " | " << sStatus << endl;
            }
            cout << "Total Rows Fetched: " << count << endl;
        }
        
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        SQLDisconnect(sqlConn);
    } else {
        cout << "DB Connection Failed.\n";
    }
    return 0;
}
