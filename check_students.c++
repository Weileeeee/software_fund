#include <iostream>
#include <sqlext.h>
#include <sqltypes.h>
#include <sql.h>

using namespace std;

int main() {
    SQLHENV sqlEnvHandle;
    SQLHDBC sqlConnHandle;
    SQLHSTMT sqlStmtHandle;
    
    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &sqlEnvHandle);
    SQLSetEnvAttr(sqlEnvHandle, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
    SQLAllocHandle(SQL_HANDLE_DBC, sqlEnvHandle, &sqlConnHandle);
    
    string connStr = "DRIVER={MySQL ODBC 9.1 Unicode Driver};SERVER=127.0.0.1;DATABASE=CampusSecurityDB;USER=root;PASSWORD=Gyl2005.;";
    SQLCHAR retConString[1024];
    
    cout << "Connecting to database..." << endl;
    if (!SQL_SUCCEEDED(SQLDriverConnectA(sqlConnHandle, NULL, (SQLCHAR*)connStr.c_str(), SQL_NTS, retConString, 1024, NULL, SQL_DRIVER_NOPROMPT))) {
        cout << "Failed to connect to database!" << endl;
        return 1;
    }
    
    cout << "Connected successfully!\n" << endl;
    
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle);
    
    // Query all students with their phone numbers
    string query = "SELECT full_name, block, whatsapp_number, is_hostel_resident FROM users WHERE role IN ('Student', 'student') ORDER BY block;";
    
    if (SQL_SUCCEEDED(SQLExecDirectA(sqlStmtHandle, (SQLCHAR*)query.c_str(), SQL_NTS))) {
        cout << "========================================" << endl;
        cout << "STUDENT PHONE NUMBERS IN DATABASE" << endl;
        cout << "========================================" << endl;
        
        char name[100], block[50], phone[20];
        int isHostel;
        SQLLEN phoneIndicator, blockIndicator;
        int count = 0;
        
        while (SQLFetch(sqlStmtHandle) == SQL_SUCCESS) {
            SQLGetData(sqlStmtHandle, 1, SQL_C_CHAR, name, sizeof(name), NULL);
            SQLGetData(sqlStmtHandle, 2, SQL_C_CHAR, block, sizeof(block), &blockIndicator);
            SQLGetData(sqlStmtHandle, 3, SQL_C_CHAR, phone, sizeof(phone), &phoneIndicator);
            SQLGetData(sqlStmtHandle, 4, SQL_C_LONG, &isHostel, 0, NULL);
            
            count++;
            cout << count << ". " << name;
            if (blockIndicator != SQL_NULL_DATA) {
                cout << " (Block " << block << ")";
            }
            cout << " - Hostel: " << (isHostel ? "Yes" : "No");
            
            if (phoneIndicator != SQL_NULL_DATA) {
                cout << " - Phone: " << phone;
            } else {
                cout << " - Phone: [NULL]";
            }
            cout << endl;
        }
        
        cout << "\nTotal students: " << count << endl;
        cout << "========================================" << endl;
    }
    
    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
    SQLDisconnect(sqlConnHandle);
    SQLFreeHandle(SQL_HANDLE_DBC, sqlConnHandle);
    SQLFreeHandle(SQL_HANDLE_ENV, sqlEnvHandle);
    
    cout << "\nPress Enter to exit...";
    cin.get();
    return 0;
}
