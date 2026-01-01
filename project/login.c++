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

// --- CONFIGURATION ---
const string DB_HOST = "127.0.0.1";
const string DB_NAME = "CampusSecurityDB";
const string DB_USER = "root";
const string DB_PASS = "admin@1234"; 
const int SERVER_PORT = 8080;

// --- DATABASE HELPERS ---

// 1. LOGIN
bool AttemptLogin(SQLHDBC sqlConnHandle, string user, string pass, string &outFullName) {
    SQLHSTMT hStmt;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &hStmt);
    
    // We only need the Name now
    string query = "SELECT full_name FROM Users WHERE username = '" + user + "' AND password_hash = '" + pass + "';";
    
    if (SQLExecDirectA(hStmt, (SQLCHAR*)query.c_str(), SQL_NTS) != SQL_SUCCESS) { 
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt); 
        return false; 
    }

    if (SQLFetch(hStmt) == SQL_SUCCESS) {
        char nameBuf[100];
        SQLGetData(hStmt, 1, SQL_C_CHAR, nameBuf, sizeof(nameBuf), NULL);
        outFullName = nameBuf;
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt); 
        return true;
    }
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt); 
    return false;
}

// 2. SAVE INCIDENT (FIXED: Uses reporter_name)
bool SaveIncident(SQLHDBC sqlConnHandle, string type, string loc, string time, string lat, string lng, string reporterName) {
    SQLHSTMT hStmt;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &hStmt);

    cout << "[DEBUG] Saving Incident by: " << reporterName << endl;

    // FIX: Column is reporter_name, and we ADD QUOTES around reporterName because it is text
    string query = "INSERT INTO Incidents (reporter_name, incident_type, location_name, reported_at, incident_status, latitude, longitude, description) VALUES ('" 
                   + reporterName + "', '" + type + "', '" + loc + "', CONCAT(CURDATE(), ' ', '" + time + ":00'), 'Pending', " + lat + ", " + lng + ", 'Reported via Web');";

    if (SQLExecDirectA(hStmt, (SQLCHAR*)query.c_str(), SQL_NTS) == SQL_SUCCESS) {
        cout << "[SUCCESS] Saved to DB." << endl;
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return true;
    } else {
        SQLCHAR sqlState[1024], message[1024];
        SQLGetDiagRecA(SQL_HANDLE_STMT, hStmt, 1, sqlState, NULL, message, 1024, NULL);
        cout << "[DB ERROR] " << message << endl;
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return false;
    }
}

// 3. FETCH ALL INCIDENTS
string GetAllIncidents(SQLHDBC sqlConnHandle) {
    SQLHSTMT hStmt;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &hStmt);
    
    // Updated to select reporter_name just in case needed later, though frontend uses type/loc
    string query = "SELECT incident_id, incident_type, location_name, DATE_FORMAT(reported_at, '%H:%i'), incident_status, latitude, longitude FROM Incidents;";

    if (SQLExecDirectA(hStmt, (SQLCHAR*)query.c_str(), SQL_NTS) != SQL_SUCCESS) return "";

    string result = "";
    char id[20], type[100], loc[100], time[50], status[50], lat[50], lng[50];
    
    while (SQLFetch(hStmt) == SQL_SUCCESS) {
        SQLGetData(hStmt, 1, SQL_C_CHAR, id, sizeof(id), NULL);
        SQLGetData(hStmt, 2, SQL_C_CHAR, type, sizeof(type), NULL);
        SQLGetData(hStmt, 3, SQL_C_CHAR, loc, sizeof(loc), NULL);
        SQLGetData(hStmt, 4, SQL_C_CHAR, time, sizeof(time), NULL);
        SQLGetData(hStmt, 5, SQL_C_CHAR, status, sizeof(status), NULL);
        SQLGetData(hStmt, 6, SQL_C_CHAR, lat, sizeof(lat), NULL);
        SQLGetData(hStmt, 7, SQL_C_CHAR, lng, sizeof(lng), NULL);
        
        result += string(id) + "|" + type + "|" + loc + "|" + time + "|" + status + "|" + lat + "|" + lng + ";";
    }
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    return result;
}

// 4. UPDATE STATUS
bool UpdateIncidentStatus(SQLHDBC sqlConnHandle, string id, string newStatus) {
    SQLHSTMT hStmt;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &hStmt);
    string query = "UPDATE Incidents SET incident_status='" + newStatus + "' WHERE incident_id=" + id + ";";
    bool success = (SQLExecDirectA(hStmt, (SQLCHAR*)query.c_str(), SQL_NTS) == SQL_SUCCESS);
    if(success) cout << "[DB] Updated ID " << id << " to " << newStatus << endl;
    else cout << "[DB ERROR] Failed to update status." << endl;
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    return success;
}

// --- UTILS ---
string LoadFile(string filename) {
    ifstream file(filename, ios::binary);
    if (!file) return "";
    stringstream buffer; buffer << file.rdbuf(); return buffer.str();
}

void ParseData(string body, string &outType, string &outLoc, string &outTime, string &outLat, string &outLng, string &outID, string &outAction) {
    auto getVal = [&](string key) {
        size_t s = body.find(key + "="); if (s == string::npos) return string("");
        s += key.length() + 1; size_t e = body.find("&", s); if (e == string::npos) e = body.length();
        string v = body.substr(s, e - s); for(char &c:v) if(c=='+')c=' '; return v;
    };
    outType = getVal("type"); outLoc = getVal("location"); outTime = getVal("time");
    outLat = getVal("lat"); outLng = getVal("lng");
    outID = getVal("id"); outAction = getVal("action");
}

int main() {
    SQLHENV sqlEnv; SQLHDBC sqlConn;
    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &sqlEnv);
    SQLSetEnvAttr(sqlEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
    SQLAllocHandle(SQL_HANDLE_DBC, sqlEnv, &sqlConn);
    string connStr = "Driver={MySQL ODBC 9.5 Unicode Driver};Server=" + DB_HOST + ";Database=" + DB_NAME + ";User=" + DB_USER + ";Password=" + DB_PASS + ";Option=3;";
    if (!SQL_SUCCEEDED(SQLDriverConnectA(sqlConn, NULL, (SQLCHAR*)connStr.c_str(), SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT))) {
        cout << "[FATAL] DB Connection Failed" << endl; return -1;
    }

    WSADATA wsa; WSAStartup(MAKEWORD(2, 2), &wsa);
    SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr; addr.sin_family = AF_INET; addr.sin_addr.s_addr = INADDR_ANY; addr.sin_port = htons(SERVER_PORT);
    bind(s, (sockaddr*)&addr, sizeof(addr)); listen(s, SOMAXCONN);
    cout << "Server Running..." << endl;
    ShellExecuteA(0, 0, "http://localhost:8080", 0, 0, SW_SHOW);

    string currentUserName = "Guest";

    while (true) {
        SOCKET c = accept(s, NULL, NULL);
        char buf[4096] = {0}; recv(c, buf, 4096, 0);
        string req(buf), resp, type, loc, time, lat, lng, id, action;

        if (req.find("GET / ") != string::npos || req.find("GET /login.html") != string::npos) {
            resp = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n" + LoadFile("login.html");
        } 
        else if (req.find("POST /login") != string::npos) {
            string body = req.substr(req.find("\r\n\r\n") + 4);
            string u, p; 
            size_t us = body.find("userid=")+7, ue = body.find("&", us); u = body.substr(us, ue-us);
            size_t ps = body.find("password=")+9; p = body.substr(ps);
            
            // PASS Variable by Reference
            if (AttemptLogin(sqlConn, u, p, currentUserName)) {
                cout << "Login Success: " << currentUserName << endl;
                resp = "HTTP/1.1 303 See Other\r\nLocation: /home.html\r\n\r\n";
            }
            else resp = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n" + LoadFile("login.html") + "<script>alert('Fail');</script>";
        }
        else if (req.find("GET /home.html") != string::npos) {
            string h = LoadFile("home.html"); 
            size_t p = h.find("Welcome, User"); if(p!=string::npos) h.replace(p,13,"Welcome, "+currentUserName);
            resp = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n" + h;
        }
        else if (req.find("POST /report_incident") != string::npos) {
            ParseData(req.substr(req.find("\r\n\r\n") + 4), type, loc, time, lat, lng, id, action);
            
            // PASS currentUserName to SaveIncident
            if(SaveIncident(sqlConn, type, loc, time, lat, lng, currentUserName)) resp = "HTTP/1.1 200 OK\r\n\r\nSaved";
            else resp = "HTTP/1.1 500 Error\r\n\r\nFail";
        }
        else if (req.find("GET /get_incidents") != string::npos) {
            string data = GetAllIncidents(sqlConn);
            resp = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n" + data;
        }
        else if (req.find("POST /update_incident") != string::npos) {
            ParseData(req.substr(req.find("\r\n\r\n") + 4), type, loc, time, lat, lng, id, action);
            string status = (action == "approve") ? "Approved" : "Rejected";
            if(UpdateIncidentStatus(sqlConn, id, status)) resp = "HTTP/1.1 200 OK\r\n\r\nUpdated";
            else resp = "HTTP/1.1 500 Error\r\n\r\nFail";
        }
        else if (req.find("GET /dashboard.css") != string::npos) resp = "HTTP/1.1 200 OK\r\nContent-Type: text/css\r\n\r\n" + LoadFile("dashboard.css");
        else if (req.find("GET /script.js") != string::npos) resp = "HTTP/1.1 200 OK\r\nContent-Type: application/javascript\r\n\r\n" + LoadFile("script.js");

        send(c, resp.c_str(), resp.length(), 0); closesocket(c);
    }
    return 0;
}