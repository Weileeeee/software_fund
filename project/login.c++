#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

// --- IMPORTANT: HEADER ORDER MATTERS ---
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h> // Must be AFTER winsock2.h
// ---------------------------------------

#include <sqlext.h>
#include <sqltypes.h>
#include <sql.h>

#pragma comment(lib, "ws2_32.lib") 

using namespace std;

// --- CONFIG ---
const string DB_HOST = "127.0.0.1";
const string DB_NAME = "CampusSecurityDB";
const string DB_USER = "root";
const string DB_PASS = "admin@1234"; 
const int SERVER_PORT = 8080;

// --- DB HELPER ---
bool AttemptLogin(SQLHDBC sqlConnHandle, string user, string pass) {
    SQLHSTMT sqlStmtHandle;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle);
    string query = "SELECT user_id FROM Users WHERE username = '" + user + "' AND password_hash = '" + pass + "';";
    if (SQLExecDirectA(sqlStmtHandle, (SQLCHAR*)query.c_str(), SQL_NTS) != SQL_SUCCESS) {
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle); return false;
    }
    bool success = (SQLFetch(sqlStmtHandle) == SQL_SUCCESS);
    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
    return success;
}

// --- FILE HELPER ---
string LoadHTMLFile(string filename) {
    ifstream file(filename);
    if (!file) return "<h1>404 Error - File Not Found (" + filename + ")</h1>";
    stringstream buffer; buffer << file.rdbuf(); return buffer.str();
}

// --- PARSER HELPER ---
void ParsePostData(string body, string &outUser, string &outPass) {
    string u_key = "userid=", p_key = "password=";
    size_t u_pos = body.find(u_key), p_pos = body.find(p_key);
    if (u_pos != string::npos) {
        size_t end = body.find("&", u_pos);
        outUser = body.substr(u_pos + u_key.length(), end - (u_pos + u_key.length()));
    }
    if (p_pos != string::npos) outPass = body.substr(p_pos + p_key.length());
}

int main() {
    // 1. SETUP DATABASE
    SQLHENV sqlEnvHandle; SQLHDBC sqlConnHandle;
    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &sqlEnvHandle);
    SQLSetEnvAttr(sqlEnvHandle, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
    SQLAllocHandle(SQL_HANDLE_DBC, sqlEnvHandle, &sqlConnHandle);

    string connString = "Driver={MySQL ODBC 9.5 Unicode Driver};Server=" + DB_HOST + 
                        ";Database=" + DB_NAME + ";User=" + DB_USER + ";Password=" + DB_PASS + ";Option=3;";

    if (!SQL_SUCCEEDED(SQLDriverConnectA(sqlConnHandle, NULL, (SQLCHAR*)connString.c_str(), SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT))) {
        cout << "[ERROR] Database Connection Failed." << endl; return -1;
    }

    // 2. SETUP SERVER
    WSADATA wsaData; WSAStartup(MAKEWORD(2, 2), &wsaData);
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY; 
    serverAddr.sin_port = htons(SERVER_PORT);

    bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(serverSocket, SOMAXCONN);

    cout << "Server started on Port " << SERVER_PORT << "..." << endl;

    // --- NEW FEATURE: AUTO-OPEN BROWSER ---
    // This command tells Windows to open the default browser to your local server
    cout << "Launching Browser..." << endl;
    ShellExecute(0, 0, "http://localhost:8080", 0, 0, SW_SHOW);
    // --------------------------------------

    // 3. LISTEN LOOP
    while (true) {
        SOCKET clientSocket = accept(serverSocket, NULL, NULL);
        char buffer[4096] = {0};
        recv(clientSocket, buffer, 4096, 0);
        string request(buffer);
        string response;

        if (request.find("GET / ") != string::npos || request.find("GET /login.html") != string::npos) {
            string html = LoadHTMLFile("login.html");
            response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n" + html;
        } 
        else if (request.find("POST /login") != string::npos) {
            string body = request.substr(request.find("\r\n\r\n") + 4);
            string u, p; ParsePostData(body, u, p);
            
            if (AttemptLogin(sqlConnHandle, u, p)) {
                // Login Success -> Go to Dashboard
                response = "HTTP/1.1 303 See Other\r\nLocation: /index.html\r\n\r\n";
            } else {
                // Login Fail -> Reload with Alert
                string html = LoadHTMLFile("login.html");
                html += "<script>alert('Login Failed: Check Credentials');</script>";
                response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n" + html;
            }
        }
        else if (request.find("GET /index.html") != string::npos) {
            string html = LoadHTMLFile("index.html");
            response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n" + html;
        }
        else if (request.find("GET /template.css") != string::npos) {
            string css = LoadHTMLFile("template.css");
            response = "HTTP/1.1 200 OK\r\nContent-Type: text/css\r\n\r\n" + css;
        }

        send(clientSocket, response.c_str(), response.length(), 0);
        closesocket(clientSocket);
    }

    closesocket(serverSocket);
    WSACleanup();
    SQLDisconnect(sqlConnHandle);
    SQLFreeHandle(SQL_HANDLE_DBC, sqlConnHandle);
    SQLFreeHandle(SQL_HANDLE_ENV, sqlEnvHandle);
    return 0;
}