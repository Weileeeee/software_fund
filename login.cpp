#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <winsock2.h>
#include <windows.h>
#include <sqlext.h>
#include <sqltypes.h>
#include <sql.h>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

// ================= CONFIG =================
const string DB_HOST = "127.0.0.1";
const string DB_NAME = "CampusSecurityDB";
const string DB_USER = "root";
const string DB_PASS = "sieN_062";
const int SERVER_PORT = 8080;  
// =========================================

string LoadFile(const string& filename) {
    ifstream file(filename, ios::binary);
    if (!file.is_open()) {
        return "<html><body><h1 style='color:red'>Server Error: Cannot open file [" + filename + "]</h1></body></html>";
    }
    stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool AttemptLogin(SQLHDBC conn, string user, string pass, string& outName) {
    SQLHSTMT stmt;
    SQLAllocHandle(SQL_HANDLE_STMT, conn, &stmt);
    string q = "SELECT full_name FROM Users WHERE username='" + user + "' AND password_hash='" + pass + "'";

    if (SQLExecDirectA(stmt, (SQLCHAR*)q.c_str(), SQL_NTS) != SQL_SUCCESS) {
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return false;
    }

    if (SQLFetch(stmt) == SQL_SUCCESS) {
        char buf[100];
        SQLGetData(stmt, 1, SQL_C_CHAR, buf, sizeof(buf), NULL);
        outName = buf;
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return true;
    }

    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    return false;
}

int main() {
    SQLHENV env;
    SQLHDBC dbc;
    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);
    SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
    SQLAllocHandle(SQL_HANDLE_DBC, env, &dbc);

    string connStr = "Driver={MySQL ODBC 8.0 Unicode Driver};Server=" + DB_HOST + ";Database=" + DB_NAME + ";User=" + DB_USER + ";Password=" + DB_PASS + ";Option=3;";

    if (!SQL_SUCCEEDED(SQLDriverConnectA(dbc, NULL, (SQLCHAR*)connStr.c_str(), SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT))) {
        cout << "[FATAL] Database connection failed!" << endl;
        return -1;
    }

    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
    SOCKET server = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(SERVER_PORT);

    bind(server, (sockaddr*)&addr, sizeof(addr));
    listen(server, 5);

    cout << "Server running at http://127.0.0.1:" << SERVER_PORT << endl;
    ShellExecuteA(NULL, "open", ("http://127.0.0.1:" + to_string(SERVER_PORT)).c_str(), NULL, NULL, SW_SHOW);

    string currentUser = "Guest";

    while (true) {
        SOCKET client = accept(server, NULL, NULL);
        if (client == INVALID_SOCKET) continue;

        char buf[4096]{};
        int bytes = recv(client, buf, sizeof(buf), 0);
        if (bytes <= 0) { closesocket(client); continue; }

        string req(buf, bytes);
        string resp;

        if (req.find("GET / ") != string::npos || req.find("GET /login.html") != string::npos) {
            resp = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n" + LoadFile("login.html");
        }
        else if (req.find("POST /login") != string::npos) {
            string body = req.substr(req.find("\r\n\r\n") + 4);
            size_t us = body.find("userid=") + 7;
            size_t ue = body.find("&", us);
            size_t ps = body.find("password=") + 9;
            string u = body.substr(us, ue - us);
            string p = body.substr(ps);

            if (AttemptLogin(dbc, u, p, currentUser)) {
                resp = "HTTP/1.1 303 See Other\r\nLocation: /home.html\r\n\r\n";
            } else {
                resp = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n" + LoadFile("login.html") + "<script>alert('Login Failed');</script>";
            }
        }
        else if (req.find("GET /home.html") != string::npos) {
            // Serve the single merged file
            string page = LoadFile("home.html");
            
            // Inject the user's name into the greeting
            size_t pos = page.find("Welcome, User");
            if (pos != string::npos) page.replace(pos, 13, "Welcome, " + currentUser);

            resp = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n" + page;
        }
        else if (req.find("GET /dashboard.css") != string::npos) {
            resp = "HTTP/1.1 200 OK\r\nContent-Type: text/css\r\n\r\n" + LoadFile("dashboard.css");
        }
        else if (req.find("GET /script.js") != string::npos) {
            resp = "HTTP/1.1 200 OK\r\nContent-Type: application/javascript\r\n\r\n" + LoadFile("script.js");
        }
        else {
            resp = "HTTP/1.1 404 Not Found\r\n\r\n<h1>404 Not Found</h1>";
        }

        send(client, resp.c_str(), resp.size(), 0);
        closesocket(client);
    }
    return 0;
}