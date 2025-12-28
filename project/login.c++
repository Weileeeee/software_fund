#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

// --- HEADER ORDER MATTERS ---
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h> 
// ----------------------------

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
bool AttemptLogin(SQLHDBC sqlConnHandle, string user, string pass, string &outFullName) {
    SQLHSTMT sqlStmtHandle;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle);

    string query = "SELECT full_name FROM Users WHERE username = '" + user + "' AND password_hash = '" + pass + "';";

    if (SQLExecDirectA(sqlStmtHandle, (SQLCHAR*)query.c_str(), SQL_NTS) != SQL_SUCCESS) {
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle); 
        return false;
    }

    if (SQLFetch(sqlStmtHandle) == SQL_SUCCESS) {
        char nameBuffer[100];
        SQLGetData(sqlStmtHandle, 1, SQL_C_CHAR, nameBuffer, sizeof(nameBuffer), NULL);
        outFullName = nameBuffer;
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return true;
    } else {
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        return false;
    }
}

// --- FILE HELPER ---
string LoadFile(string filename) {
    ifstream file(filename, ios::binary); // Binary mode is safer for all file types
    if (!file) return "";
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
    cout << "Launching Browser..." << endl;
    
    // Auto-launch browser
    ShellExecuteA(0, 0, "http://localhost:8080", 0, 0, SW_SHOW);

    string currentUserFullName = "Guest"; 

    // 3. LISTEN LOOP
    while (true) {
        SOCKET clientSocket = accept(serverSocket, NULL, NULL);
        char buffer[4096] = {0};
        recv(clientSocket, buffer, 4096, 0);
        string request(buffer);
        string response;

        // --- ROUTING LOGIC ---

        // 1. LOGIN PAGE
        if (request.find("GET / ") != string::npos || request.find("GET /login.html") != string::npos) {
            string html = LoadFile("login.html");
            response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n" + html;
        } 
        // 2. PROCESS LOGIN
        else if (request.find("POST /login") != string::npos) {
            string body = request.substr(request.find("\r\n\r\n") + 4);
            string u, p; ParsePostData(body, u, p);
            
            if (AttemptLogin(sqlConnHandle, u, p, currentUserFullName)) {
                cout << "Login Success: " << currentUserFullName << endl;
                response = "HTTP/1.1 303 See Other\r\nLocation: /home.html\r\n\r\n";
            } else {
                string html = LoadFile("login.html");
                html += "<script>alert('Login Failed: Invalid Credentials');</script>";
                response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n" + html;
            }
        }
        // 3. HOME PAGE (Dashboard)
        else if (request.find("GET /home.html") != string::npos) {
            string html = LoadFile("home.html");
            // Inject User Name
            string placeholder = "Welcome, Gan Yu Lun";
            size_t pos = html.find(placeholder);
            if (pos != string::npos) {
                html.replace(pos, placeholder.length(), "Welcome, " + currentUserFullName);
            }
            response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n" + html;
        }
        // 4. CSS FILE
        else if (request.find("GET /dashboard.css") != string::npos) {
            string css = LoadFile("dashboard.css");
            response = "HTTP/1.1 200 OK\r\nContent-Type: text/css\r\n\r\n" + css;
        }
        // 5. JAVASCRIPT FILE (NEW - Critical for Tabs/Map)
        else if (request.find("GET /script.js") != string::npos) {
            string js = LoadFile("script.js");
            response = "HTTP/1.1 200 OK\r\nContent-Type: application/javascript\r\n\r\n" + js;
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