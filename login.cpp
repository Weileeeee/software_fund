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

// ================= URL DECODE =================
string urlDecode(const string& str) {
    string result;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '%') {
            if (i + 2 < str.length()) {
                int value;
                stringstream ss;
                ss << hex << str.substr(i + 1, 2);
                ss >> value;
                result += static_cast<char>(value);
                i += 2;
            }
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }
    return result;
}

// ================= CONFIG =================
const string DB_HOST = "127.0.0.1";
const string DB_NAME = "CampusSecurityDB";
const string DB_USER = "root";
const string DB_PASS = "sieN_062";
const int SERVER_PORT = 8080;
// =========================================

// ================= FILE LOADER =================
string LoadFile(const string& filename) {
    ifstream file(filename, ios::binary);
    if (!file.is_open()) {
        return "<h1>Cannot open " + filename + "</h1>";
    }
    stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// ================= LOGIN =================
bool AttemptLogin(SQLHDBC conn, string user, string pass, string& outName) {
    SQLHSTMT stmt;
    SQLAllocHandle(SQL_HANDLE_STMT, conn, &stmt);

    string q =
        "SELECT full_name FROM Users "
        "WHERE username='" + user + "' AND password_hash='" + pass + "'";

    if (!SQL_SUCCEEDED(SQLExecDirectA(stmt, (SQLCHAR*)q.c_str(), SQL_NTS))) {
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return false;
    }

    SQLRETURN r = SQLFetch(stmt);
    if (SQL_SUCCEEDED(r)) {
        char buf[256] = {0};
        SQLLEN ind = 0;
        SQLGetData(stmt, 1, SQL_C_CHAR, buf, sizeof(buf), &ind);
        if (ind != SQL_NULL_DATA) outName = buf;
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return true;
    }

    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    return false;
}

// ================= GET BROADCASTS =================
string GetBroadcasts(SQLHDBC dbc) {
    SQLHSTMT stmt;
    SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);

    string result;
    string q = "SELECT id, to_group, message, sent_at, sent_by FROM Broadcasts ORDER BY sent_at DESC";

    if (SQL_SUCCEEDED(SQLExecDirectA(stmt, (SQLCHAR*)q.c_str(), SQL_NTS))) {
        SQLRETURN r = SQLFetch(stmt);
        while (SQL_SUCCEEDED(r)) {
            SQLLEN ind = 0;
            char id[64] = {0};
            char to_group[256] = {0};
            char message[1024] = {0};
            char sent_at[64] = {0};
            char sent_by[256] = {0};

            SQLGetData(stmt, 1, SQL_C_CHAR, id, sizeof(id), &ind);
            SQLGetData(stmt, 2, SQL_C_CHAR, to_group, sizeof(to_group), &ind);
            SQLGetData(stmt, 3, SQL_C_CHAR, message, sizeof(message), &ind);
            SQLGetData(stmt, 4, SQL_C_CHAR, sent_at, sizeof(sent_at), &ind);
            SQLGetData(stmt, 5, SQL_C_CHAR, sent_by, sizeof(sent_by), &ind);

            result += string(id) + "|" + string(to_group) + "|" + string(message) + "|" + string(sent_at) + "|" + string(sent_by) + ";";

            r = SQLFetch(stmt);
        }
    }

    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    return result;
}

// ================= GET STUDENTS =================
string GetStudents(SQLHDBC dbc) {
    SQLHSTMT stmt;
    SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);

    string result;
    string q = "SELECT student_id, name, location, status FROM Students";

    if (SQL_SUCCEEDED(SQLExecDirectA(stmt, (SQLCHAR*)q.c_str(), SQL_NTS))) {
        SQLRETURN r = SQLFetch(stmt);
        while (SQL_SUCCEEDED(r)) {
            SQLLEN ind = 0;
            char student_id[64] = {0};
            char name[256] = {0};
            char location[256] = {0};
            char status[64] = {0};

            SQLGetData(stmt, 1, SQL_C_CHAR, student_id, sizeof(student_id), &ind);
            SQLGetData(stmt, 2, SQL_C_CHAR, name, sizeof(name), &ind);
            SQLGetData(stmt, 3, SQL_C_CHAR, location, sizeof(location), &ind);
            SQLGetData(stmt, 4, SQL_C_CHAR, status, sizeof(status), &ind);

            result += string(student_id) + "|" + string(name) + "|" + string(location) + "|" + string(status) + ";";

            r = SQLFetch(stmt);
        }
    }

    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    return result;
}

// ================= MAIN =================
int main() {
    // ---- DB ----
    SQLHENV env;
    SQLHDBC dbc;

    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);
    SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
    SQLAllocHandle(SQL_HANDLE_DBC, env, &dbc);

    string connStr =
        "Driver={MySQL ODBC 8.0 Unicode Driver};"
        "Server=" + DB_HOST +
        ";Database=" + DB_NAME +
        ";User=" + DB_USER +
        ";Password=" + DB_PASS + ";";

    if (!SQL_SUCCEEDED(SQLDriverConnectA(
            dbc, NULL, (SQLCHAR*)connStr.c_str(),
            SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT))) {
        cout << "[FATAL] Database connection failed!" << endl;
        return -1;
    }

    // ---- SOCKET ----
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
    ShellExecuteA(NULL, "open",
        ("http://127.0.0.1:" + to_string(SERVER_PORT)).c_str(),
        NULL, NULL, SW_SHOW);

    string currentUser = "Guest";

    while (true) {
        SOCKET client = accept(server, NULL, NULL);
        if (client == INVALID_SOCKET) continue;

        char buf[8192]{};
        int bytes = recv(client, buf, sizeof(buf), 0);
        if (bytes <= 0) { closesocket(client); continue; }

        string req(buf, bytes);
        string resp;

        // ---------- ROUTES ----------
        if (req.find("GET / ") != string::npos ||
            req.find("GET /login.html") != string::npos) {
            resp = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
                   + LoadFile("login.html");
        }

        else if (req.find("POST /login") != string::npos) {
            string body = req.substr(req.find("\r\n\r\n") + 4);
            string u = body.substr(body.find("userid=") + 7,
                                   body.find("&") - (body.find("userid=") + 7));
            string p = body.substr(body.find("password=") + 9);

            if (AttemptLogin(dbc, u, p, currentUser)) {
                resp = "HTTP/1.1 303 See Other\r\nLocation: /home.html\r\n\r\n";
            } else {
                resp = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
                       + LoadFile("login.html")
                       + "<script>alert('Login Failed');</script>";
            }
        }

        else if (req.find("GET /home.html") != string::npos) {
            string page = LoadFile("home.html");
            size_t pos = page.find("Welcome, User");
            if (pos != string::npos)
                page.replace(pos, 13, "Welcome, " + currentUser);

            resp = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n" + page;
        }

        else if (req.find("GET /get_broadcasts") != string::npos) {
            string data = GetBroadcasts(dbc);
            resp = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n" + data;
        }

        else if (req.find("GET /get_students") != string::npos) {
            string data = GetStudents(dbc);
            resp = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n" + data;
        }

        else if (req.find("POST /broadcast") != string::npos) {
            string body = req.substr(req.find("\r\n\r\n") + 4);
            // Parse body: to=...&message=...
            string to_group, message;
            size_t pos = body.find("to=");
            if (pos != string::npos) {
                to_group = body.substr(pos + 3, body.find("&") - (pos + 3));
            }
            pos = body.find("message=");
            if (pos != string::npos) {
                message = body.substr(pos + 8);
            }
            to_group = urlDecode(to_group);
            message = urlDecode(message);
            // INSERT INTO Broadcasts (to_group, message, sent_at, sent_by) VALUES (...)
            SQLHSTMT stmt;
            SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);
            string q = "INSERT INTO Broadcasts (to_group, message, sent_at, sent_by) VALUES ('" + to_group + "', '" + message + "', NOW(), '" + currentUser + "')";
            SQLExecDirectA(stmt, (SQLCHAR*)q.c_str(), SQL_NTS);
            SQLFreeHandle(SQL_HANDLE_STMT, stmt);
            resp = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nBroadcast saved.";
        }

        else if (req.find("POST /delete_broadcast") != string::npos) {
            string body = req.substr(req.find("\r\n\r\n") + 4);
            string id;
            size_t pos = body.find("id=");
            if (pos != string::npos) {
                id = body.substr(pos + 3);
            }
            // DELETE FROM Broadcasts WHERE id=?
            SQLHSTMT stmt;
            SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);
            string q = "DELETE FROM Broadcasts WHERE id=" + id;
            SQLExecDirectA(stmt, (SQLCHAR*)q.c_str(), SQL_NTS);
            SQLFreeHandle(SQL_HANDLE_STMT, stmt);
            resp = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nBroadcast deleted.";
        }

        else if (req.find("POST /update_student_status") != string::npos) {
            string body = req.substr(req.find("\r\n\r\n") + 4);
            // Parse body: student_id=status&...
            // For simplicity, assume one: student_id=1201101563&status=Safe
            string student_id, status;
            size_t pos = body.find("student_id=");
            if (pos != string::npos) {
                student_id = body.substr(pos + 11, body.find("&") - (pos + 11));
            }
            pos = body.find("status=");
            if (pos != string::npos) {
                status = body.substr(pos + 7);
            }
            cout << "Updating student " << student_id << " to " << status << endl;
            // UPDATE Students SET status=? WHERE student_id=?
            SQLHSTMT stmt;
            SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);
            string q = "UPDATE Students SET status='" + status + "' WHERE student_id='" + student_id + "'";
            if (SQL_SUCCEEDED(SQLExecDirectA(stmt, (SQLCHAR*)q.c_str(), SQL_NTS))) {
                resp = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nStatus updated.";
            } else {
                resp = "HTTP/1.1 500 Internal Server Error\r\n\r\nFailed to update student status.";
            }
            SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        }

        else if (req.find("GET /dashboard.css") != string::npos) {
            resp = "HTTP/1.1 200 OK\r\nContent-Type: text/css\r\n\r\n"
                   + LoadFile("dashboard.css");
        }

        else if (req.find("GET /script.js") != string::npos) {
            resp = "HTTP/1.1 200 OK\r\nContent-Type: application/javascript\r\n\r\n"
                   + LoadFile("script.js");
        }

        else {
            resp = "HTTP/1.1 404 Not Found\r\n\r\n404";
        }

        send(client, resp.c_str(), resp.size(), 0);
        closesocket(client);
    }
}
