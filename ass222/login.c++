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

// --- 配置区域 ---
const string DB_HOST = "127.0.0.1";
const string DB_NAME = "CampusSecurityDB";
const string DB_USER = "root";
const string DB_PASS = "Soh@20050531";  // 你的数据库密码
const int SERVER_PORT = 8080;

string currentUserName = "Guest";
string currentUserRole = "student"; 

// --- 工具函数：获取当前 EXE 所在的文件夹路径 ---
// 用于解决“找不到 python 脚本”的问题
string GetCurrentDir() {
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    string::size_type pos = string(buffer).find_last_of("\\/");
    return string(buffer).substr(0, pos);
}

// --- 数据库辅助函数 ---

// 1. 登录
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

// 2. 保存报案
bool SaveIncident(SQLHDBC sqlConnHandle, string type, string loc, string time, string lat, string lng, string reporterName) {
    SQLHSTMT hStmt;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &hStmt);

    cout << "[DEBUG] Saving Incident by: " << reporterName << endl;

    string query = "INSERT INTO Incidents (reporter_name, incident_type, location_name, reported_at, incident_status, latitude, longitude, description) VALUES ('" 
                   + reporterName + "', '" + type + "', '" + loc + "', CONCAT(CURDATE(), ' ', '" + time + ":00'), 'Pending', " + lat + ", " + lng + ", 'Reported via Web');";

    if (SQLExecDirectA(hStmt, (SQLCHAR*)query.c_str(), SQL_NTS) == SQL_SUCCESS) {
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return true;
    }
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    return false;
}

// 3. 获取所有案件
string GetAllIncidents(SQLHDBC sqlConnHandle) {
    SQLHSTMT hStmt;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &hStmt);
    
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

// 4. 获取案件详情 (用于发邮件获取类型和地点)
bool GetIncidentDetails(SQLHDBC sqlConnHandle, string incidentId, string &outType, string &outLoc) {
    SQLHSTMT hStmt;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &hStmt);
    
    string query = "SELECT incident_type, location_name FROM Incidents WHERE incident_id = " + incidentId + ";";

    if (SQLExecDirectA(hStmt, (SQLCHAR*)query.c_str(), SQL_NTS) != SQL_SUCCESS) {
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return false;
    }

    if (SQLFetch(hStmt) == SQL_SUCCESS) {
        char typeBuf[100], locBuf[100];
        SQLGetData(hStmt, 1, SQL_C_CHAR, typeBuf, sizeof(typeBuf), NULL);
        SQLGetData(hStmt, 2, SQL_C_CHAR, locBuf, sizeof(locBuf), NULL);
        outType = typeBuf;
        outLoc = locBuf;
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return true;
    }
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    return false;
}

// 5. 【新功能】获取所有学生邮箱
string GetAllStudentEmails(SQLHDBC sqlConnHandle) {
    SQLHSTMT hStmt;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &hStmt);
    
    // 查询所有角色为 Student 的邮箱
    string query = "SELECT email FROM Users WHERE role = 'Student';";
    
    if (SQLExecDirectA(hStmt, (SQLCHAR*)query.c_str(), SQL_NTS) != SQL_SUCCESS) {
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return "";
    }

    string allEmails = "";
    char emailBuf[100];
    
    while (SQLFetch(hStmt) == SQL_SUCCESS) {
        SQLGetData(hStmt, 1, SQL_C_CHAR, emailBuf, sizeof(emailBuf), NULL);
        string e = string(emailBuf);
        if (!e.empty()) {
            allEmails += e + ","; // 拼接逗号
        }
    }
    
    // 去除末尾多余逗号
    if (!allEmails.empty() && allEmails.back() == ',') {
        allEmails.pop_back();
    }

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    return allEmails;
}

// 6. 更新状态并【群发广播】
bool UpdateIncidentStatus(SQLHDBC sqlConnHandle, string id, string newStatus) {
    SQLHSTMT hStmt;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &hStmt);
    string query = "UPDATE Incidents SET incident_status='" + newStatus + "' WHERE incident_id=" + id + ";";
    
    bool success = (SQLExecDirectA(hStmt, (SQLCHAR*)query.c_str(), SQL_NTS) == SQL_SUCCESS);
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);

    // ========================================================
    // 📧 广播通知逻辑 (Broadcast)
    // ========================================================
    if (success && newStatus == "Approved") {
        string type, loc;
        
        // 1. 获取案件详情 (用于邮件内容)
        if (GetIncidentDetails(sqlConnHandle, id, type, loc)) {
            
            // 2. 获取【全校学生】邮箱列表
            string allRecipients = GetAllStudentEmails(sqlConnHandle);
            
            if (!allRecipients.empty()) {
                cout << "------------------------------------------------" << endl;
                cout << "[BROADCAST] Target Emails: " << allRecipients << endl;
                
                // 3. 准备调用 Python
                // 获取当前 EXE 路径，确保能找到 py 文件
                string currentDir = GetCurrentDir();
                string scriptPath = currentDir + "\\send_mail.py";
                
                cout << "[BROADCAST] Launching Python script: " << scriptPath << endl;
                
                // 4. 构造命令
                // python "script_path" "email1,email2..." "Fire" "Block A"
                string cmd = "python \"" + scriptPath + "\" \"" + allRecipients + "\" \"" + type + "\" \"" + loc + "\"";
                
                // 5. 执行 (前台运行，方便看报错)
                int res = system(cmd.c_str());
                
                if(res == 0) cout << "[BROADCAST] Python script finished successfully." << endl;
                else cout << "[BROADCAST] ERROR: Python script failed. Code: " << res << endl;
                cout << "------------------------------------------------" << endl;
            } else {
                cout << "[BROADCAST] No students found in database." << endl;
            }

        } else {
            cout << "[BROADCAST ERROR] Could not find incident details." << endl;
        }
    }

    if(success) cout << "[DB] Updated ID " << id << " to " << newStatus << endl;
    else cout << "[DB ERROR] Failed to update status." << endl;
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
    cout << "Server Running at http://localhost:8080" << endl;
    ShellExecuteA(0, 0, "http://localhost:8080", 0, 0, SW_SHOW);

    while (true) {
        SOCKET c = accept(s, NULL, NULL);
        char buf[4096] = {0}; recv(c, buf, 4096, 0);
        string req(buf), resp, type, loc, time, lat, lng, id, action;

        // POST /login 处理
        if (req.find("POST /login") != string::npos) {
            string body = req.substr(req.find("\r\n\r\n") + 4);
            string u, p; 
            size_t us = body.find("userid=")+7, ue = body.find("&", us); u = body.substr(us, ue-us);
            size_t ps = body.find("password=")+9;
            size_t pe = body.find("&", ps);
            if (pe == string::npos) p = body.substr(ps); else p = body.substr(ps, pe - ps);
            
            // 简单解码 @ 符号
            size_t at_sign = p.find("%40");
            if (at_sign != string::npos) p.replace(at_sign, 3, "@");

            if (AttemptLogin(sqlConn, u, p, currentUserName, currentUserRole)) {
                cout << "Login Success. User: " << currentUserName << " | Role: " << currentUserRole << endl;
                resp = "HTTP/1.1 303 See Other\r\nLocation: /home.html\r\n\r\n";
            }
            else resp = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n" + LoadFile("login.html") + "<script>alert('Login Failed');</script>";
        }
        else if (req.find("GET / ") != string::npos || req.find("GET /login.html") != string::npos) {
            resp = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n" + LoadFile("login.html");
        } 
        else if (req.find("GET /home.html") != string::npos) {
            string h = LoadFile("home.html"); 
            string roleScript = "<script>window.SERVER_INJECTED_ROLE = '" + currentUserRole + "';</script>";
            size_t rp = h.find(""); 
            if(rp != string::npos) h.replace(rp, 23, roleScript);
            size_t p = h.find("Welcome, User"); 
            if(p != string::npos) h.replace(p, 13, "Welcome, " + currentUserName);
            resp = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n" + h;
        }
        else if (req.find("POST /report_incident") != string::npos) {
            ParseData(req.substr(req.find("\r\n\r\n") + 4), type, loc, time, lat, lng, id, action);
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
            // 触发更新 + 邮件
            if(UpdateIncidentStatus(sqlConn, id, status)) resp = "HTTP/1.1 200 OK\r\n\r\nUpdated";
            else resp = "HTTP/1.1 500 Error\r\n\r\nFail";
        }
        else if (req.find("GET /dashboard.css") != string::npos) resp = "HTTP/1.1 200 OK\r\nContent-Type: text/css\r\n\r\n" + LoadFile("dashboard.css");
        else if (req.find("GET /script.js") != string::npos) resp = "HTTP/1.1 200 OK\r\nContent-Type: application/javascript\r\n\r\n" + LoadFile("script.js");

        send(c, resp.c_str(), resp.length(), 0); closesocket(c);
    }
    return 0;
}