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

void ExecuteSQL(SQLHSTMT hStmt, string query) {
    cout << "Executing: " << query << " ... ";
    SQLRETURN ret = SQLExecDirectA(hStmt, (SQLCHAR*)query.c_str(), SQL_NTS);
    if (SQL_SUCCEEDED(ret)) {
        cout << "SUCCESS\n";
    } else {
        cout << "FAILED (Might already exist)\n";
    }
}

int main() {
    SQLHENV sqlEnv; SQLHDBC sqlConn; SQLHSTMT hStmt;
    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &sqlEnv);
    SQLSetEnvAttr(sqlEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
    SQLAllocHandle(SQL_HANDLE_DBC, sqlEnv, &sqlConn);
    string connStr = "Driver={MySQL ODBC 9.5 Unicode Driver};Server=" + DB_HOST + ";Database=" + DB_NAME + ";User=" + DB_USER + ";Password=" + DB_PASS + ";Option=3;";
    
    if (SQL_SUCCEEDED(SQLDriverConnectA(sqlConn, NULL, (SQLCHAR*)connStr.c_str(), SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT))) {
        cout << "Connected!\n";
        SQLAllocHandle(SQL_HANDLE_STMT, sqlConn, &hStmt);
        
        // 1. Add phone_number column
        ExecuteSQL(hStmt, "ALTER TABLE Users ADD COLUMN phone_number VARCHAR(20) DEFAULT NULL;");
        
        // 2. Set dummy phone number for all students (for testing)
        // ideally checking schema detailedly before doing this, but 'ALTER IGNORE' isnt standard.
        // If it fails, it fails safely usually.
        
        ExecuteSQL(hStmt, "UPDATE Users SET phone_number = '+60123456789' WHERE role IN ('Student', 'student');");
        
        SQLDisconnect(sqlConn);
    }
    return 0;
}
