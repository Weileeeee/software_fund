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
#pragma comment(lib, "odbc32.lib")

using namespace std;

// =============================================================
// ⚙️ CONFIGURATION
// =============================================================
const string DB_HOST = "127.0.0.1";
const string DB_NAME = "CampusSecurityDB";
const string DB_USER = "root";
const string DB_PASS = "admin@1234"; // Update with your actual password
const int SERVER_PORT = 8080;

// GLOBAL STATE
string currentUserName = "Guest";
string currentUserRole = "student"; 
string currentUserBlock = "";

// =============================================================
// 🛠️ HELPER FUNCTIONS
// =============================================================

// Get Current Directory (to find python script)
string GetCurrentDir() {
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    string::size_type pos = string(buffer).find_last_of("\\/");
    return string(buffer).substr(0, pos);
}

// Read File Content
string LoadFile(string filename) {
    ifstream file(filename, ios::binary);
    if (!file) return "";
    stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Parse URL Parameter
string GetP(string b, string k) {
    size_t s = b.find(k + "="); if (s == string::npos) return "";
    s += k.length() + 1; 
    size_t e = b.find("&", s); 
    if (e == string::npos) e = b.length();
    string v = b.substr(s, e - s); 
    
    // URL Decode (Basic)
    string decoded;
    for (size_t i = 0; i < v.length(); ++i) {
        if (v[i] == '+') decoded += ' ';
        else if (v[i] == '%' && i + 2 < v.length()) {
            string hex = v.substr(i + 1, 2);
            char c = (char)strtol(hex.c_str(), NULL, 16);
            decoded += c;
            i += 2;
        } else {
            decoded += v[i];
        }
    }
    return decoded;
}

// Helper to parse all common POST data at once
void ParseData(string body, string &outType, string &outLoc, string &outTime, string &outLat, string &outLng, string &outID, string &outAction, string &outBlock) {
    outType = GetP(body, "type");
    outLoc = GetP(body, "location");
    outTime = GetP(body, "time");
    outLat = GetP(body, "lat");
    outLng = GetP(body, "lng");
    outID = GetP(body, "id");
    outAction = GetP(body, "action");
    outBlock = GetP(body, "block");
}

// =============================================================
// 🗄️ DATABASE & LOGIC FUNCTIONS
// =============================================================

// 1. Login Logic
bool AttemptLogin(SQLHDBC sqlConnHandle, string user, string pass, string &outFullName, string &outRole, string &outBlock) {
    SQLHSTMT hStmt;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &hStmt);
    string query = "SELECT full_name, role, IFNULL(block, '') FROM Users WHERE username = '" + user + "' AND password_hash = '" + pass + "';";
    if (SQLExecDirectA(hStmt, (SQLCHAR*)query.c_str(), SQL_NTS) != SQL_SUCCESS) { SQLFreeHandle(SQL_HANDLE_STMT, hStmt); return false; }
    if (SQLFetch(hStmt) == SQL_SUCCESS) {
        char nameBuf[100], roleBuf[50], blockBuf[50];
        SQLGetData(hStmt, 1, SQL_C_CHAR, nameBuf, sizeof(nameBuf), NULL);
        SQLGetData(hStmt, 2, SQL_C_CHAR, roleBuf, sizeof(roleBuf), NULL);
        SQLGetData(hStmt, 3, SQL_C_CHAR, blockBuf, sizeof(blockBuf), NULL);
        outFullName = nameBuf; outRole = roleBuf; outBlock = blockBuf;
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt); return true;
    }
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt); return false;
}

// 2. Incident Reporting
string SaveIncident(SQLHDBC sqlConnHandle, string type, string loc, string time, string lat, string lng, string desc, string sev, string evid, string reporterName) {
    SQLHSTMT hStmt;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &hStmt);
    
    // Handle optional Lat/Lng
    if(lat.empty()) lat = "0.0";
    if(lng.empty()) lng = "0.0";

    // 1. Insert into Main Incidents Table
    string query = "INSERT INTO Incidents (reporter_name, incident_type, location_name, reported_at, incident_status, latitude, longitude, description, severity, evidence_path) VALUES ('" 
                   + reporterName + "', '" + type + "', '" + loc + "', CONCAT(CURDATE(), ' ', '" + time + ":00'), 'Pending', " + lat + ", " + lng + ", '" + desc + "', '" + sev + "', '" + evid + "');";
    
    if (SQLExecDirectA(hStmt, (SQLCHAR*)query.c_str(), SQL_NTS) != SQL_SUCCESS) {
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return ""; // Fail
    }

    // 2. Get the Generated Incident ID
    string newId = "0";
    SQLExecDirectA(hStmt, (SQLCHAR*)"SELECT LAST_INSERT_ID();", SQL_NTS);
    if(SQLFetch(hStmt) == SQL_SUCCESS) {
        char idBuf[20];
        SQLGetData(hStmt, 1, SQL_C_CHAR, idBuf, sizeof(idBuf), NULL);
        newId = idBuf;
    }
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt); // Free to reset cursor

    // 3. Insert into Warden Reports Table (Specific Log)
    // We assume any report via this function falls under this requirement, or check roles if needed. 
    // Given the flow fits "Warden logs new incident", we log it here.
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &hStmt);
    string wardenQuery = "INSERT INTO wardenreports (incident_id, warden_name, report_details) VALUES (" + newId + ", '" + reporterName + "', '" + desc + "');";
    SQLExecDirectA(hStmt, (SQLCHAR*)wardenQuery.c_str(), SQL_NTS);
    
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    return newId;
}

// 3. Get Incidents List
string GetAllIncidents(SQLHDBC sqlConnHandle) {
    SQLHSTMT hStmt;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &hStmt);
    // Include ID in response
    string query = "SELECT incident_id, incident_type, location_name, DATE_FORMAT(reported_at, '%H:%i'), incident_status, latitude, longitude FROM Incidents;";
    if (SQLExecDirectA(hStmt, (SQLCHAR*)query.c_str(), SQL_NTS) != SQL_SUCCESS) {
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return "";
    }

    string result = "";
    char id[20], type[50], loc[100], time[20], status[50], lat[20], lng[20];
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

// --- NEW RESIDENCE FUNCTIONS ---
string GetResidentList(SQLHDBC sqlConnHandle, string block) {
    SQLHSTMT hStmt;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &hStmt);
    // Fetch residents for the specific block. 
    // FORCE FIX: Ignore block filter entirely for now so the user can see their test data.
    // CHANGED: Format Room as "Block-Room"
    // CHANGED: Only show Hostel Residents
    string query = "SELECT user_id, full_name, CONCAT(IFNULL(block, '?'), '-', IFNULL(room_number, '?')), IFNULL(stay_status, 'Present'), email "
                   "FROM Users WHERE role = 'Student' AND is_hostel_resident = 1;";
    
    if (SQLExecDirectA(hStmt, (SQLCHAR*)query.c_str(), SQL_NTS) != SQL_SUCCESS) {
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return "";
    }

    string result = "";
    char id[20], nm[100], rm[50], st[50], em[100]; // Increased rm buffer for "Block-Room"
    while (SQLFetch(hStmt) == SQL_SUCCESS) {
        SQLGetData(hStmt, 1, SQL_C_CHAR, id, 20, NULL);
        SQLGetData(hStmt, 2, SQL_C_CHAR, nm, 100, NULL);
        SQLGetData(hStmt, 3, SQL_C_CHAR, rm, 50, NULL);
        SQLGetData(hStmt, 4, SQL_C_CHAR, st, 50, NULL);
        SQLGetData(hStmt, 5, SQL_C_CHAR, em, 100, NULL);
        result += string(id) + "|" + nm + "|" + rm + "|" + st + "|" + em + ";";
    }
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    return result;
}

bool UpdateStayStatus(SQLHDBC sqlConnHandle, string id, string status) {
    SQLHSTMT hStmt;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &hStmt);
    string query = "UPDATE Users SET stay_status='" + status + "' WHERE user_id=" + id + ";";
    bool success = (SQLExecDirectA(hStmt, (SQLCHAR*)query.c_str(), SQL_NTS) == SQL_SUCCESS);
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    return success;
}

// 4. Email Helpers (From File 2)
bool GetIncidentDetails(SQLHDBC sqlConnHandle, string incidentId, string &outType, string &outLoc) {
    SQLHSTMT hStmt;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &hStmt);
    string query = "SELECT incident_type, location_name FROM Incidents WHERE incident_id = " + incidentId + ";";
    if (SQLExecDirectA(hStmt, (SQLCHAR*)query.c_str(), SQL_NTS) != SQL_SUCCESS) { SQLFreeHandle(SQL_HANDLE_STMT, hStmt); return false; }
    if (SQLFetch(hStmt) == SQL_SUCCESS) {
        char typeBuf[100], locBuf[100];
        SQLGetData(hStmt, 1, SQL_C_CHAR, typeBuf, sizeof(typeBuf), NULL);
        SQLGetData(hStmt, 2, SQL_C_CHAR, locBuf, sizeof(locBuf), NULL);
        outType = typeBuf; outLoc = locBuf;
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt); return true;
    }
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt); return false;
}

// Helper to trim whitespace/newlines
string Trim(const string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (string::npos == first) return str;
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}

string GetAllStudentEmails(SQLHDBC sqlConnHandle) {
    SQLHSTMT hStmt;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &hStmt);
    string query = "SELECT email FROM Users WHERE role = 'Student';";
    if (SQLExecDirectA(hStmt, (SQLCHAR*)query.c_str(), SQL_NTS) != SQL_SUCCESS) return "";
    
    string allEmails = "";
    char emailBuf[100];
    while (SQLFetch(hStmt) == SQL_SUCCESS) {
        SQLGetData(hStmt, 1, SQL_C_CHAR, emailBuf, sizeof(emailBuf), NULL);
        string e = Trim(string(emailBuf)); // Clean the email
        if (!e.empty()) allEmails += e + ",";
    }
    if (!allEmails.empty() && allEmails.back() == ',') allEmails.pop_back();
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    return allEmails;
}
// 5. Update Status + Trigger Python Broadcast
//
bool UpdateIncidentStatus(SQLHDBC sqlConnHandle, string id, string newStatus) {
    SQLHSTMT hStmt;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &hStmt);
    string query = "UPDATE Incidents SET incident_status='" + newStatus + "' WHERE incident_id=" + id + ";";
    bool success = (SQLExecDirectA(hStmt, (SQLCHAR*)query.c_str(), SQL_NTS) == SQL_SUCCESS);
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    
    if (success && newStatus == "Approved") {
        string type, loc;
        if (GetIncidentDetails(sqlConnHandle, id, type, loc)) {
            string allRecipients = GetAllStudentEmails(sqlConnHandle);
            
            // Clean inputs to prevent command breakage
            type = Trim(type);
            loc = Trim(loc);

            if (!allRecipients.empty()) {
                cout << "[BROADCAST] Emailing students: " << allRecipients << endl;
                
                string scriptPath = GetCurrentDir() + "\\send_mail.py";
                
                // Construct command explicitly with cmd /c to handle paths better
                string cmd = "cmd /c python \"" + scriptPath + "\" \"" + allRecipients + "\" \"" + type + "\" \"" + loc + "\"";
                
                // Debug: Print the exact command being run
                cout << "[DEBUG] Running Command: " << cmd << endl;
                
                int result = system(cmd.c_str());
                if(result != 0) cout << "[ERROR] Python script failed with code: " << result << endl;
            }
        }
    }
    return success;
}

// 6. Evacuation Logic (From File 1)
bool StartEvacuation(SQLHDBC sqlConnHandle, string block) {
    SQLHSTMT hStmt;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &hStmt);
    cout << "[EVACUATION] Triggered for: " << (block.empty() ? "ALL HOSTELS" : block) << endl;
    
    string query;
    if (block.empty() || block == "ALL") {
        // CASE A: Evacuate EVERYONE in a hostel (Global Alarm)
        // CHANGED: Filter by is_hostel_resident = 1
        query = "UPDATE EvacuationLogs e JOIN Users u ON e.user_id = u.user_id "
                "SET e.status = 'Missing' " // Missing = Unsafe
                "WHERE u.role = 'Student' AND u.is_hostel_resident = 1;";
    } else {
        // CASE B: Evacuate specific block only
        // CHANGED: Filter by is_hostel_resident = 1
        query = "UPDATE EvacuationLogs e JOIN Users u ON e.user_id = u.user_id "
                "SET e.status = 'Missing' "
                "WHERE u.role = 'Student' AND u.block = '" + block + "' AND u.is_hostel_resident = 1;";
    }

    bool success = (SQLExecDirectA(hStmt, (SQLCHAR*)query.c_str(), SQL_NTS) == SQL_SUCCESS);
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    return success;
}

void GenerateUnsafeReport(SQLHDBC sqlConnHandle) {
    SQLHSTMT hStmt;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &hStmt);
    string query = "SELECT u.full_name, IFNULL(u.block,'-'), IFNULL(u.username,'-') FROM Users u JOIN EvacuationLogs e ON u.user_id = e.user_id WHERE e.status = 'Missing';";
    
    if (SQLExecDirectA(hStmt, (SQLCHAR*)query.c_str(), SQL_NTS) == SQL_SUCCESS) {
        ofstream report("Unsafe_Students.txt");
        report << "UNSAFE STUDENT REPORT\n---------------------\n";
        char name[100], block[50], code[50];
        while (SQLFetch(hStmt) == SQL_SUCCESS) {
            SQLGetData(hStmt, 1, SQL_C_CHAR, name, sizeof(name), NULL);
            SQLGetData(hStmt, 2, SQL_C_CHAR, block, sizeof(block), NULL);
            SQLGetData(hStmt, 3, SQL_C_CHAR, code, sizeof(code), NULL);
            report << name << " (Block " << block << ") - ID: " << code << "\n";
        }
        report.close();
        ShellExecuteA(NULL, "open", "Unsafe_Students.txt", NULL, NULL, SW_SHOWNORMAL);
    }
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
}

string GetEvacuationList(SQLHDBC sqlConnHandle) {
    SQLHSTMT hStmt;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &hStmt);
    
    // CHANGED: Format Block as "Block-Room" using CONCAT for Evacuation List
    // CHANGED: Filter by is_hostel_resident = 1
    string query = "SELECT u.user_id, u.full_name, CONCAT(IFNULL(u.block, '?'), '-', IFNULL(u.room_number, '?')), IFNULL(e.status, 'Safe'), IFNULL(DATE_FORMAT(e.last_updated, '%H:%i:%s'), '-') "
                   "FROM Users u LEFT JOIN EvacuationLogs e ON u.user_id = e.user_id "
                   "WHERE u.role = 'Student' AND u.is_hostel_resident = 1;";

    if (SQLExecDirectA(hStmt, (SQLCHAR*)query.c_str(), SQL_NTS) != SQL_SUCCESS) return "";
    
    string result = "";
    char id[20], name[100], block[20], status[20], time[50];
    while (SQLFetch(hStmt) == SQL_SUCCESS) {
        SQLGetData(hStmt, 1, SQL_C_CHAR, id, sizeof(id), NULL);
        SQLGetData(hStmt, 2, SQL_C_CHAR, name, sizeof(name), NULL);
        SQLGetData(hStmt, 3, SQL_C_CHAR, block, sizeof(block), NULL);
        SQLGetData(hStmt, 4, SQL_C_CHAR, status, sizeof(status), NULL);
        SQLGetData(hStmt, 5, SQL_C_CHAR, time, sizeof(time), NULL);
        result += string(id) + "|" + name + "|" + block + "|" + status + "|" + time + ";";
    }
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    return result;
}

bool MarkStudentSafe(SQLHDBC sqlConnHandle, string userId) {
    SQLHSTMT hStmt;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &hStmt);
    string query = "UPDATE EvacuationLogs SET status='Safe' WHERE user_id=" + userId + ";";
    bool success = (SQLExecDirectA(hStmt, (SQLCHAR*)query.c_str(), SQL_NTS) == SQL_SUCCESS);
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    return success;
}

string GetUserIdByName(SQLHDBC sqlConnHandle, string username) {
    SQLHSTMT hStmt;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &hStmt);
    string query = "SELECT user_id FROM Users WHERE username = '" + username + "'";
    string id = "0";
    if (SQLExecDirectA(hStmt, (SQLCHAR*)query.c_str(), SQL_NTS) == SQL_SUCCESS) {
        if(SQLFetch(hStmt) == SQL_SUCCESS) {
            char buf[50]; SQLGetData(hStmt, 1, SQL_C_CHAR, buf, sizeof(buf), NULL);
            id = buf;
        }
    }
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    return id;
}

// =============================================================
// 🚀 MAIN SERVER LOOP
// =============================================================

int main() {
    // Database Connection
    SQLHENV sqlEnv; SQLHDBC sqlConn;
    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &sqlEnv);
    SQLSetEnvAttr(sqlEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
    SQLAllocHandle(SQL_HANDLE_DBC, sqlEnv, &sqlConn);
    string connStr = "Driver={MySQL ODBC 9.5 Unicode Driver};Server=" + DB_HOST + ";Database=" + DB_NAME + ";User=" + DB_USER + ";Password=" + DB_PASS + ";Option=3;";
    
    if (!SQL_SUCCEEDED(SQLDriverConnectA(sqlConn, NULL, (SQLCHAR*)connStr.c_str(), SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT))) {
        cout << "[FATAL] DB Connection Failed" << endl; return -1;
    }

    // Socket Setup
    WSADATA wsa; WSAStartup(MAKEWORD(2, 2), &wsa);
    SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr; addr.sin_family = AF_INET; addr.sin_addr.s_addr = INADDR_ANY; addr.sin_port = htons(SERVER_PORT);
    bind(s, (sockaddr*)&addr, sizeof(addr)); listen(s, SOMAXCONN);
    
    cout << "Server Running at http://localhost:" << SERVER_PORT << endl;
    ShellExecuteA(0, 0, ("http://localhost:" + to_string(SERVER_PORT)).c_str(), 0, 0, SW_SHOW);

    while (true) {
        SOCKET c = accept(s, NULL, NULL);
        char buf[8192] = {0}; 
        recv(c, buf, 8192, 0);
        string req(buf);
        string body = "";
        
        // Split header and body
        size_t bodyPos = req.find("\r\n\r\n");
        if(bodyPos != string::npos) body = req.substr(bodyPos + 4);

        string resp = "HTTP/1.1 404 Not Found\r\n\r\n";

        // ROUTING
        if (req.find("GET / ") != string::npos || req.find("GET /login.html") != string::npos) {
            resp = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n" + LoadFile("login.html");
        } 
        else if (req.find("POST /login") != string::npos) {
            string u = GetP(body, "userid"); string p = GetP(body, "password");
            if (AttemptLogin(sqlConn, u, p, currentUserName, currentUserRole, currentUserBlock)) {
                cout << "Login: " << currentUserName << " (" << currentUserRole << ")" << endl;
                resp = "HTTP/1.1 303 See Other\r\nLocation: /home.html\r\n\r\n";
            } else resp = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n" + LoadFile("login.html") + "<script>alert('Login Failed');</script>";
        }
        else if (req.find("GET /home.html") != string::npos) {
            string h = LoadFile("home.html"); 
            string rs = "<script>window.SERVER_INJECTED_ROLE = '" + currentUserRole + "';</script>";
            size_t rp = h.find("<body>"); if(rp != string::npos) h.insert(rp + 6, rs);
            size_t p = h.find("Welcome, User"); if(p!=string::npos) h.replace(p,13,"Welcome, "+currentUserName);
            resp = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n" + h;
        }
        else if (req.find("GET /script.js") != string::npos) {
            resp = "HTTP/1.1 200 OK\r\nContent-Type: application/javascript\r\n\r\n" + LoadFile("script.js");
        }
        else if (req.find("GET /dashboard.css") != string::npos) {
            resp = "HTTP/1.1 200 OK\r\nContent-Type: text/css\r\n\r\n" + LoadFile("dashboard.css");
        }
        // RESIDENCE MONITORING
        else if (req.find("GET /get_residents") != string::npos) {
            resp = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n" + GetResidentList(sqlConn, currentUserBlock);
        }
        else if (req.find("POST /update_stay_status") != string::npos) {
            if(UpdateStayStatus(sqlConn, GetP(body, "id"), GetP(body, "status"))) resp = "HTTP/1.1 200 OK\r\n\r\nUpdated";
            else resp = "HTTP/1.1 500 Error\r\n\r\nFail";
        }
        // INCIDENTS
        else if (req.find("POST /report_incident") != string::npos) {
            string type, loc, time, lat, lng, desc, sev, evid;
            ParseData(body, type, loc, time, lat, lng, desc, sev, evid);
            string newId = SaveIncident(sqlConn, type, loc, time, lat, lng, desc, sev, evid, currentUserName);
            if(newId != "") resp = "HTTP/1.1 200 OK\r\n\r\n" + newId; // Return ID
            else resp = "HTTP/1.1 500 Error\r\n\r\nFail";
        }
        else if (req.find("GET /get_incidents") != string::npos) {
            resp = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n" + GetAllIncidents(sqlConn);
        }
        else if (req.find("POST /update_incident") != string::npos) {
            string status = (GetP(body,"action") == "approve") ? "Approved" : "Rejected";
            if(UpdateIncidentStatus(sqlConn, GetP(body,"id"), status)) resp = "HTTP/1.1 200 OK\r\n\r\nUpdated";
            else resp = "HTTP/1.1 500 Error\r\n\r\nFail";
        }
        // EVACUATION
        else if (req.find("GET /get_evacuation") != string::npos) {
            resp = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n" + GetEvacuationList(sqlConn);
        }
        else if (req.find("POST /start_evacuation") != string::npos) {
            if(StartEvacuation(sqlConn, GetP(body,"block"))) resp = "HTTP/1.1 200 OK\r\n\r\nStarted";
            else resp = "HTTP/1.1 500 Error\r\n\r\nFail";
        }
        else if (req.find("POST /generate_report") != string::npos) {
            GenerateUnsafeReport(sqlConn);
            resp = "HTTP/1.1 200 OK\r\n\r\nGenerated";
        }
        else if (req.find("POST /mark_safe") != string::npos) {
            if(MarkStudentSafe(sqlConn, GetP(body,"id"))) resp = "HTTP/1.1 200 OK\r\n\r\nUpdated";
            else resp = "HTTP/1.1 500 Error\r\n\r\nFail";
        }
        else if (req.find("POST /self_safe") != string::npos) {
            string myID = GetUserIdByName(sqlConn, currentUserName);
            if(MarkStudentSafe(sqlConn, myID)) resp = "HTTP/1.1 200 OK\r\n\r\nUpdated";
            else resp = "HTTP/1.1 500 Error\r\n\r\nFail";
        }

        send(c, resp.c_str(), resp.length(), 0); closesocket(c);
    }
    return 0;
}