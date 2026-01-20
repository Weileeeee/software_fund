#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h> 
#include <sqlext.h>
#include <sqltypes.h>
#include <sql.h>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

const string DB_HOST = "127.0.0.1";
const string DB_NAME = "CampusSecurityDB";
const string DB_USER = "root";
const string DB_PASS = "Gyl2005.";
const int SERVER_PORT = 8080;

string currentUserName = "Guest";
string currentUserRole = "student";

// ========== DATABASE LOGIN ==========
bool AttemptLogin(SQLHDBC sqlConnHandle, string user, string pass, string &outFullName, string &outRole) {
    SQLHSTMT hStmt;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &hStmt);

    string query = "SELECT full_name, role FROM Users WHERE username = '" + user + "' AND password_hash = '" + pass + "';";

    if (SQLExecDirectA(hStmt, (SQLCHAR*)query.c_str(), SQL_NTS) != SQL_SUCCESS) {
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return false;
    }

    if (SQLFetch(hStmt) == SQL_SUCCESS) {
        char nameBuf[100], roleBuf[100];
        SQLGetData(hStmt, 1, SQL_C_CHAR, nameBuf, sizeof(nameBuf), NULL);
        SQLGetData(hStmt, 2, SQL_C_CHAR, roleBuf, sizeof(roleBuf), NULL);
        outFullName = nameBuf;
        outRole = roleBuf;
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return true;
    }

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    return false;
}

string LoadFile(string filename) {
    ifstream file(filename, ios::binary);
    if (!file) return "";
    stringstream buffer; buffer << file.rdbuf();
    return buffer.str();
}

int main() {

    // Initialize DB
    SQLHENV sqlEnv; SQLHDBC sqlConn;
    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &sqlEnv);
    SQLSetEnvAttr(sqlEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
    SQLAllocHandle(SQL_HANDLE_DBC, sqlEnv, &sqlConn);

    string connStr = "Driver={MySQL ODBC 9.5 Unicode Driver};Server=" + DB_HOST +
                     ";Database=" + DB_NAME + ";User=" + DB_USER +
                     ";Password=" + DB_PASS + ";Option=3;";

    if (!SQL_SUCCEEDED(SQLDriverConnectA(sqlConn, NULL, (SQLCHAR*)connStr.c_str(), SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT))) {
        cout << "[DB ERROR] Connection Failed" << endl;
        return -1;
    }

    // Initialize Socket
    WSADATA wsa; WSAStartup(MAKEWORD(2,2), &wsa);
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(SERVER_PORT);

    bind(serverSocket, (sockaddr*)&addr, sizeof(addr));
    listen(serverSocket, SOMAXCONN);

    cout << "Server Running on http://localhost:8080" << endl;

    while (true) {
        SOCKET client = accept(serverSocket, NULL, NULL);

        char buf[4096] = {0};
        recv(client, buf, 4096, 0);

        string req(buf);
        string resp;

        // ========= LOGIN HANDLER ==========
        if (req.find("POST /login") != string::npos) {
            string body = req.substr(req.find("\r\n\r\n") + 4);

            auto getVal = [&](string key) -> string {
                size_t p = body.find(key + "=");
                if (p == string::npos) return "";
                size_t s = p + key.size() + 1;
                size_t e = body.find("&", s);
                if (e == string::npos) e = body.size();
                return body.substr(s, e - s);
            };

            string username = getVal("username");
            string password = getVal("password");

            cout << "[LOGIN ATTEMPT] " << username << ":" << password << endl;

            if (AttemptLogin(sqlConn, username, password, currentUserName, currentUserRole)) {
                cout << "[LOGIN SUCCESS] User=" << currentUserName << " Role=" << currentUserRole << endl;
                resp = "HTTP/1.1 303 See Other\r\nLocation: /home.html\r\n\r\n";
            } else {
                cout << "[LOGIN FAILED]" << endl;
                resp = "HTTP/1.1 200 OK\r\nContent-Type:text/plain\r\n\r\nLogin Failed";
            }
        }
        else if (req.find("GET /home.html") != string::npos) {
            resp = "HTTP/1.1 200 OK\r\nContent-Type:text/html\r\n\r\n" + LoadFile("home.html");
        }
        else {
            resp = "HTTP/1.1 200 OK\r\nContent-Type:text/html\r\n\r\n" + LoadFile("login.html");
        }

        send(client, resp.c_str(), resp.size(), 0);
        closesocket(client);
    }

    return 0;
}
