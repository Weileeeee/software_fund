#include <windows.h>
#include <winhttp.h>
#include <iostream>
#include <string>
#include <vector>
#include <sqlext.h>
#include <sqltypes.h>
#include <sql.h>

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "odbc32.lib")

using namespace std;

// ---------------------------
// Database Configuration
// ---------------------------
const string DB_HOST = "127.0.0.1";
const string DB_NAME = "CampusSecurityDB";
const string DB_USER = "root";
const string DB_PASS = "你的数据库密码"; // 改成你的密码

// ---------------------------
// WhatsApp Configuration
// ---------------------------
const string PHONE_NUMBER_ID = "962408263622848";
const string ACCESS_TOKEN = "EAAjQvZASvqfkBQlZAZAPE5SrzjOaZCsiH6CSeygb7EkFjZAV3YQjYfK4akhmAJ2ZB0Qe0bEBjMiEfwIzC5Xxoc3jSWbIbg7yyOaSFkXSTANLPgFuZBD8HgusLpJ6tzvjuqzStWt8KYjvkKTNPOm4VcIlQgaNUhNDfLT1FX1syHIzKJ1dPdlA85pDU4ZBb14QS9xL2F5OoZBRnJ7hazQqWtvZCt2Lv7NU9MJA30ZACTPq4HZAbRjFmkKu6wazPfkMbzY5nUlp2IvV97AE6MNYLZClCggMmc5QTugZDZD";

// ---------------------------
// WhatsApp API Function
// ---------------------------
bool SendWhatsAppMessage(const std::string& phoneNumberId,
                         const std::string& token,
                         const std::string& recipient,
                         const std::string& messageText) 
{
    std::wstring host = L"graph.facebook.com";
    std::wstring path = L"/v20.0/" + std::wstring(phoneNumberId.begin(), phoneNumberId.end()) + L"/messages";

    HINTERNET hSession = WinHttpOpen(L"WhatsAppSender/1.0",
                                     WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                     WINHTTP_NO_PROXY_NAME,
                                     WINHTTP_NO_PROXY_BYPASS,
                                     0);

    if (!hSession) return false;

    HINTERNET hConnect = WinHttpConnect(hSession,
                                        host.c_str(),
                                        INTERNET_DEFAULT_HTTPS_PORT,
                                        0);
    if (!hConnect) return false;

    HINTERNET hRequest = WinHttpOpenRequest(hConnect,
                                            L"POST",
                                            path.c_str(),
                                            NULL,
                                            WINHTTP_NO_REFERER,
                                            WINHTTP_DEFAULT_ACCEPT_TYPES,
                                            WINHTTP_FLAG_SECURE);
    if (!hRequest) return false;

    // Build JSON body
    std::string json =
        "{"
        "\"messaging_product\": \"whatsapp\","
        "\"to\": \"" + recipient + "\","
        "\"type\": \"text\","
        "\"text\": { \"preview_url\": false, \"body\": \"" + messageText + "\" }"
        "}";

    // Headers
    std::string auth = "Authorization: Bearer " + token;
    WinHttpAddRequestHeaders(hRequest,
        std::wstring(auth.begin(), auth.end()).c_str(),
        (DWORD)-1L,
        WINHTTP_ADDREQ_FLAG_ADD);

    WinHttpAddRequestHeaders(hRequest,
        L"Content-Type: application/json",
        (DWORD)-1L,
        WINHTTP_ADDREQ_FLAG_ADD);

    BOOL result = WinHttpSendRequest(hRequest,
                                     WINHTTP_NO_ADDITIONAL_HEADERS,
                                     0,
                                     (LPVOID)json.c_str(),
                                     (DWORD)json.size(),
                                     (DWORD)json.size(),
                                     0);
    if (!result) return false;

    result = WinHttpReceiveResponse(hRequest, NULL);
    if (!result) return false;

    DWORD statusCode = 0;
    DWORD size = sizeof(statusCode);
    WinHttpQueryHeaders(hRequest,
                        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX,
                        &statusCode,
                        &size,
                        WINHTTP_NO_HEADER_INDEX);

    cout << "[WhatsApp API] Response Status: " << statusCode << " for " << recipient << endl;

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return (statusCode == 200 || statusCode == 201);
}

// ---------------------------
// Main Program
// ---------------------------
int main() {
    SQLHENV sqlEnv; SQLHDBC sqlConn;
    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &sqlEnv);
    SQLSetEnvAttr(sqlEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
    SQLAllocHandle(SQL_HANDLE_DBC, sqlEnv, &sqlConn);

    string connStr = "Driver={MySQL ODBC 9.5 Unicode Driver};Server=" + DB_HOST + ";Database=" + DB_NAME + ";User=" + DB_USER + ";Password=" + DB_PASS + ";Option=3;";

    if (!SQL_SUCCEEDED(SQLDriverConnectA(sqlConn, NULL, (SQLCHAR*)connStr.c_str(), SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT))) {
        cout << "[FATAL] DB Connection Failed" << endl; 
        return -1;
    }

    cout << "[INFO] Connected to database successfully!" << endl;

    // ---------------------------
    // Fetch students who have WhatsApp numbers
    // ---------------------------
    SQLHSTMT hStmt;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConn, &hStmt);
    string query = "SELECT user_id, full_name, whatsapp_number FROM Users WHERE role='Student' AND whatsapp_number IS NOT NULL AND whatsapp_number!='';";

    if (!SQL_SUCCEEDED(SQLExecDirectA(hStmt, (SQLCHAR*)query.c_str(), SQL_NTS))) {
        cout << "[ERROR] Failed to fetch users" << endl;
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return -1;
    }

    char name[100], number[50];
    int userId;

    // ---------------------------
    // Send WhatsApp to each student
    // ---------------------------
    while (SQLFetch(hStmt) == SQL_SUCCESS) {
        SQLGetData(hStmt, 1, SQL_C_SLONG, &userId, 0, NULL);
        SQLGetData(hStmt, 2, SQL_C_CHAR, name, sizeof(name), NULL);
        SQLGetData(hStmt, 3, SQL_C_CHAR, number, sizeof(number), NULL);

        string studentName(name);
        string phoneNumber(number);

        string message = "🚨 Fire Evacuation: Block A. Please follow safety instructions.";

        bool sent = SendWhatsAppMessage(PHONE_NUMBER_ID, ACCESS_TOKEN, phoneNumber, message);

        cout << "[INFO] Delivery to " << studentName << ": " << (sent ? "Sent" : "Failed") << endl;

        // 可选：在 AlertDelivery 表里插入记录
        SQLHSTMT insertStmt;
        SQLAllocHandle(SQL_HANDLE_STMT, sqlConn, &insertStmt);
        string insertQuery = "INSERT INTO AlertDelivery (alert_id, recipient_user_id, channel, delivery_status, acknowledged) "
                             "VALUES (1, " + to_string(userId) + ", 'Whatsapp', '" + string(sent ? "Sent" : "Failed") + "', 0);";
        SQLExecDirectA(insertStmt, (SQLCHAR*)insertQuery.c_str(), SQL_NTS);
        SQLFreeHandle(SQL_HANDLE_STMT, insertStmt);
    }

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);

    SQLDisconnect(sqlConn);
    SQLFreeHandle(SQL_HANDLE_DBC, sqlConn);
    SQLFreeHandle(SQL_HANDLE_ENV, sqlEnv);

    cout << "[INFO] All notifications processed." << endl;

    return 0;
}
