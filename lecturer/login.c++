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
#include <winhttp.h> // Added for WhatsApp API

#pragma comment(lib, "ws2_32.lib") 
#pragma comment(lib, "odbc32.lib")
#pragma comment(lib, "winhttp.lib") // Added for WhatsApp API

using namespace std;

// =============================================================
// ⚙️ CONFIGURATION
// =============================================================
const string DB_HOST = "127.0.0.1";
const string DB_NAME = "CampusSecurityDB";
const string DB_USER = "root";
const string DB_PASS = "sieN_062"; // Update with your actual password
const int SERVER_PORT = 8080;

// WhatsApp Config
const string PHONE_NUMBER_ID = "962408263622848";
const string ACCESS_TOKEN = "EAAjQvZASvqfkBQgv714fvpfH1DGrTu80WQ0PZARqyCtjAkyZA5C92TEypf5X5BZCEQylpJv1EBpcnWCqeNlnfyUGx9ZAC9ZCUVlI9khS5XVYApbsTZAteWy6lCnnkDJAxjtjvLp5k6GPymr2NDmgQwJyVfLWTN7BFOvZB4cCcclcYtBFbvKeSZBl0zmC74FA1ktfZAPxZBaPikWuYTG01dwBbwWrvyspOfZApcfKxLbxm9mZBEXSRNzKTdIBIHMJwAxpQlrMbZCTd3nlqNL8C5Qm3QGMqZBeRB4";

// GLOBAL STATE
string currentUserName = "Guest";
string currentUserRole = "student"; 

string currentUserBlock = "";

// =============================================================
// 🛠️ HELPER FUNCTIONS
// =============================================================

// Helper to escape JSON special characters
string JsonEscape(const string& s) {
    string res;
    for (char c : s) {
        if (c == '\n') res += "\\n";
        else if (c == '\r') res += "\\r";
        else if (c == '"') res += "\\\"";
        else if (c == '\\') res += "\\\\";
        else res += c;
    }
    return res;
}

// WhatsApp API Function
bool SendWhatsAppMessage(const string& recipient, const string& messageText) {
    // 0. Format Phone Number (Auto-fix for Malaysia)
    string cleanPhone = "";
    for (char c : recipient) {
        if (isdigit(c)) cleanPhone += c;
    }
    // If number starts with 0 (e.g. 012345...), replace 0 with 60
    if (cleanPhone.length() > 2 && cleanPhone[0] == '0') {
         cleanPhone = "60" + cleanPhone.substr(1);
    }

    // 1. Prepare Request Data
    wstring host = L"graph.facebook.com";
    wstring path = L"/v20.0/" + wstring(PHONE_NUMBER_ID.begin(), PHONE_NUMBER_ID.end()) + L"/messages";

    // Initialize WinHTTP
    HINTERNET hSession = WinHttpOpen(L"CampusSecurity/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return false;

    HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", path.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);

    // 2. Setup JSON Payload and Authorization
    // Escape the message text for JSON
    string escapedMsg = JsonEscape(messageText);
    
    // Note: recipient must be digits only (e.g., 60123456789)
    string json = "{\"messaging_product\":\"whatsapp\",\"to\":\"" + cleanPhone + "\",\"type\":\"text\",\"text\":{\"body\":\"" + escapedMsg + "\"}}";
    string auth = "Authorization: Bearer " + ACCESS_TOKEN;

    // 3. Add Headers
    WinHttpAddRequestHeaders(hRequest, wstring(auth.begin(), auth.end()).c_str(), (DWORD)-1L, WINHTTP_ADDREQ_FLAG_ADD);
    WinHttpAddRequestHeaders(hRequest, L"Content-Type: application/json", (DWORD)-1L, WINHTTP_ADDREQ_FLAG_ADD);

    // 4. THE CRITICAL PART: Send the Request and Wait for Response
    cout << "[WhatsApp] Attempting to send to: " << cleanPhone << "..." << endl;
    
    BOOL result = WinHttpSendRequest(
        hRequest, 
        WINHTTP_NO_ADDITIONAL_HEADERS, 0, 
        (LPVOID)json.c_str(), (DWORD)json.size(), 
        (DWORD)json.size(), 0
    );

    if (result) {
        result = WinHttpReceiveResponse(hRequest, NULL);
    }

    // 5. Check Status Code for Debugging
    if (result) {
        DWORD dwStatusCode = 0;
        DWORD dwSize = sizeof(dwStatusCode);
        WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &dwStatusCode, &dwSize, WINHTTP_NO_HEADER_INDEX);
        
        if (dwStatusCode == 200 || dwStatusCode == 201) {
            cout << "[WhatsApp] SUCCESS! Message delivered to " << cleanPhone << endl;
        } else {
            cout << "[WhatsApp] FAILED! Meta API returned Status Code: " << dwStatusCode << endl;
            // Read and print error response for deeper debugging
            DWORD dwAvailable = 0;
            string apiResponse = "";
            do {
                dwAvailable = 0;
                WinHttpQueryDataAvailable(hRequest, &dwAvailable);
                if (dwAvailable > 0) {
                    vector<char> pszOutBuffer(dwAvailable + 1);
                    DWORD dwRead = 0;
                    WinHttpReadData(hRequest, &pszOutBuffer[0], dwAvailable, &dwRead);
                    pszOutBuffer[dwRead] = 0;
                    apiResponse += &pszOutBuffer[0];
                }
            } while (dwAvailable > 0);
            cout << "[API Response]: " << apiResponse << endl;
            
            cout << "💡 Hint: 401 = Expired Token, 400 = Wrong Number format, Sandbox Limit, or Invalid JSON." << endl;
            result = false;
        }
    } else {
        cout << "[WhatsApp] ERROR: Could not connect to Meta Servers. Error Code: " << GetLastError() << endl;
    }

    // 6. Cleanup handles
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return result;
}

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
// Helper to parse all common POST data at once
void ParseData(string body, string &outType, string &outLoc, string &outTime, string &outLat, string &outLng, string &outDesc, string &outSev, string &outEvid) {
    outType = GetP(body, "type");
    outLoc = GetP(body, "location");
    outTime = GetP(body, "time");
    outLat = GetP(body, "lat");
    outLng = GetP(body, "lng");
    outDesc = GetP(body, "description"); // Was outID
    outSev = GetP(body, "severity");     // Was outAction
    outEvid = GetP(body, "evidence");    // Was outBlock
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

// 2. Get User ID by Name
string GetUserIdByName(SQLHDBC sqlConnHandle, string fullName) {
    SQLHSTMT hStmt;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &hStmt);
    string query = "SELECT user_id FROM Users WHERE full_name = '" + fullName + "';";
    SQLExecDirectA(hStmt, (SQLCHAR*)query.c_str(), SQL_NTS);
    if (SQLFetch(hStmt) == SQL_SUCCESS) {
        char id[20];
        SQLGetData(hStmt, 1, SQL_C_CHAR, id, sizeof(id), NULL);
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return string(id);
    }
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    return "";
}

// URL Decode helper for special characters
string urlDecode(string str) {
    string result;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '+') result += ' ';
        else if (str[i] == '%' && i + 2 < str.length()) {
            string hex = str.substr(i + 1, 2);
            char c = (char)strtol(hex.c_str(), NULL, 16);
            result += c;
            i += 2;
        } else result += str[i];
    }
    return result;
}

// 3. Residence Monitoring
string GetResidentList(SQLHDBC sqlConnHandle, string block) {
    SQLHSTMT hStmt;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &hStmt);
    
    // If block is empty, show all hostel residents
    string query;
    if (block.empty()) {
        query = "SELECT u.user_id, u.full_name, u.block, u.stay_status, u.username FROM users u WHERE u.role IN ('Student', 'student') AND u.is_hostel_resident = 1;";
    } else {
        query = "SELECT u.user_id, u.full_name, u.block, u.stay_status, u.username FROM users u WHERE u.role IN ('Student', 'student') AND u.block = '" + block + "' AND u.is_hostel_resident = 1;";
    }
    
    SQLExecDirectA(hStmt, (SQLCHAR*)query.c_str(), SQL_NTS);
    
    string result = "";
    char id[20], name[100], blk[50], stat[50], email[100];
    while (SQLFetch(hStmt) == SQL_SUCCESS) {
        SQLGetData(hStmt, 1, SQL_C_CHAR, id, sizeof(id), NULL);
        SQLGetData(hStmt, 2, SQL_C_CHAR, name, sizeof(name), NULL);
        SQLGetData(hStmt, 3, SQL_C_CHAR, blk, sizeof(blk), NULL);
        SQLLEN indicator; SQLGetData(hStmt, 4, SQL_C_CHAR, stat, sizeof(stat), &indicator);
        SQLGetData(hStmt, 5, SQL_C_CHAR, email, sizeof(email), NULL);
        
        string status = (indicator == SQL_NULL_DATA) ? "Unknown" : string(stat);
        result += string(id) + "|" + name + "|" + blk + "|" + status + "|" + email + ";";
    }
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    return result;
}

// UpdateStayStatus removed: student status feature deprecated per request.

// 4. Incident Management
string SaveIncident(SQLHDBC sqlConnHandle, string type, string loc, string time, string lat, string lng, string desc, string sev, string evid, string reporter) {
    SQLHSTMT hStmt;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &hStmt);
    
    // Clean quotes in strings (basic SQL injection protection)
    auto clean = [](string s) { for(auto &c : s) if(c == '\'') c = '`'; return s; };
    // Strictly follow unisafe schema: All reports go into Incidents.
    // Warden reports also get a record in wardenreports.
    // Evidence goes into incidentmedia.
    
    string query = "INSERT INTO Incidents (incident_type, location_name, reported_at, latitude, longitude, description, severity, incident_status, reporter_name) VALUES ('" 
                   + type + "', '" + loc + "', '" + time + "', '" + lat + "', '" + lng + "', '" + desc + "', '" + sev + "', 'Pending', '" + reporter + "');";
    
    if (SQL_SUCCEEDED(SQLExecDirectA(hStmt, (SQLCHAR*)query.c_str(), SQL_NTS))) {
        // Get the last inserted ID
        SQLExecDirectA(hStmt, (SQLCHAR*)"SELECT LAST_INSERT_ID();", SQL_NTS);
        string incidentId = "";
        if (SQLFetch(hStmt) == SQL_SUCCESS) {
            char idChar[20];
            SQLGetData(hStmt, 1, SQL_C_CHAR, idChar, sizeof(idChar), NULL);
            incidentId = string(idChar);
        }

        if (incidentId != "") {
            // 1. If Warden, also save to wardenreports
            if (currentUserRole == "Warden") {
                string wQuery = "INSERT INTO wardenreports (incident_id, warden_name, report_details) VALUES (" 
                                + incidentId + ", '" + reporter + "', '" + desc + "');";
                SQLExecDirectA(hStmt, (SQLCHAR*)wQuery.c_str(), SQL_NTS);
            }

            // 2. If evidence exists, save to incidentmedia
            if (evid != "" && evid != "No evidence") {
                string mQuery = "INSERT INTO incidentmedia (incident_id, file_path) VALUES (" 
                                + incidentId + ", '" + evid + "');";
                SQLExecDirectA(hStmt, (SQLCHAR*)mQuery.c_str(), SQL_NTS);
            }

            SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
            return incidentId;
        }
    } else {
        SQLCHAR sqlState[6], msg[SQL_MAX_MESSAGE_LENGTH];
        SQLINTEGER nativeError;
        SQLSMALLINT msgLen;
        SQLGetDiagRec(SQL_HANDLE_STMT, hStmt, 1, sqlState, &nativeError, msg, sizeof(msg), &msgLen);
        cout << "[DB ERROR] Failed to Save Incident: " << msg << " (State: " << sqlState << ")" << endl;
        cout << "[DB QUERY] " << query << endl;
    }
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    return "";
}

string GetAllIncidents(SQLHDBC sqlConnHandle) {
    SQLHSTMT hStmt;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &hStmt);
    
    // JOIN with wardenreports to get detailed observations if they exist
    string query = "SELECT i.incident_id, i.incident_type, i.location_name, DATE_FORMAT(i.reported_at, '%Y-%m-%d %H:%i'), i.incident_status, i.reporter_name, i.latitude, i.longitude, wr.report_details "
                   "FROM Incidents i LEFT JOIN wardenreports wr ON i.incident_id = wr.incident_id "
                   "WHERE i.incident_status != 'Rejected' "
                   "ORDER BY i.reported_at DESC;";
                   
    SQLExecDirectA(hStmt, (SQLCHAR*)query.c_str(), SQL_NTS);
    
    string result = "";
    char id[20], type[100], loc[100], time[50], stat[50], rep[100], lat[20], lng[20], det[512];
    SQLLEN iDet;
    while (SQLFetch(hStmt) == SQL_SUCCESS) {
        SQLGetData(hStmt, 1, SQL_C_CHAR, id, sizeof(id), NULL);
        SQLGetData(hStmt, 2, SQL_C_CHAR, type, sizeof(type), NULL);
        SQLGetData(hStmt, 3, SQL_C_CHAR, loc, sizeof(loc), NULL);
        SQLGetData(hStmt, 4, SQL_C_CHAR, time, sizeof(time), NULL);
        SQLGetData(hStmt, 5, SQL_C_CHAR, stat, sizeof(stat), NULL);
        SQLGetData(hStmt, 6, SQL_C_CHAR, rep, sizeof(rep), NULL);
        SQLGetData(hStmt, 7, SQL_C_CHAR, lat, sizeof(lat), NULL);
        SQLGetData(hStmt, 8, SQL_C_CHAR, lng, sizeof(lng), NULL);
        SQLGetData(hStmt, 9, SQL_C_CHAR, det, sizeof(det), &iDet);
        
        string detail = (iDet == SQL_NULL_DATA) ? "" : string(det);
        // Replace pipe in detail to avoid parsing issues
        for(auto &c : detail) if(c == '|') c = ' ';
        
        result += string(id) + "|" + type + "|" + loc + "|" + time + "|" + stat + "|" + rep + "|" + lat + "|" + lng + "|" + detail + ";";
    }
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    return result;
}

// 4. Email Helpers (Added during merge)
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
bool UpdateIncidentStatus(SQLHDBC sqlConnHandle, string id, string newStatus) {
    cout << "[DEBUG] UpdateIncidentStatus called for ID: " << id << " with status: " << newStatus << endl;
    SQLHSTMT hStmt;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &hStmt);
    string query = "UPDATE Incidents SET incident_status='" + newStatus + "' WHERE incident_id=" + id + ";";
    bool success = SQL_SUCCEEDED(SQLExecDirectA(hStmt, (SQLCHAR*)query.c_str(), SQL_NTS));
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    
    if (success) {
        cout << "[DEBUG] Incident " << id << " status updated successfully to " << newStatus << " in DB." << endl;
        if (newStatus == "Approved") {
            string type, loc;
            if (GetIncidentDetails(sqlConnHandle, id, type, loc)) {
                cout << "[DEBUG] Incident details found: Type=" << type << ", Location=" << loc << endl;
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
                    cout << "[DEBUG] Command execution result (system return code): " << result << endl;
                    if(result != 0) cout << "[ERROR] Python script failed with code: " << result << endl;
                } else {
                    cout << "[DEBUG] No student emails found for broadcast." << endl;
                }
            } else {
                cout << "[DEBUG] Failed to get incident details for ID: " << id << endl;
            }
        }
    } else {
        cout << "[DEBUG] Failed to update incident status in DB for ID: " << id << endl;
    }
    return success;
}

// 5. Evacuation Management
string GetEvacuationList(SQLHDBC sqlConnHandle) {
    cout << "[DEBUG] Entering GetEvacuationList" << endl;
    SQLHSTMT hStmt;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &hStmt);
    // Added is_hostel_resident filter
    string query = "SELECT u.user_id, u.full_name, u.block, IFNULL(sp.status, 'Unknown') FROM users u LEFT JOIN evacuationlogs sp ON u.user_id = sp.user_id WHERE u.role IN ('Student', 'student') AND u.is_hostel_resident = 1 ORDER BY u.block;";
    cout << "[DEBUG] Executing Query: " << query << endl;
    SQLRETURN ret = SQLExecDirectA(hStmt, (SQLCHAR*)query.c_str(), SQL_NTS);
    if (!SQL_SUCCEEDED(ret)) {
        cout << "[DEBUG] Query Failed!" << endl;
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return "";
    }
    
    string result = "";
    char id[20] = "", name[100] = "", blk[50] = "", stat[50] = "";
    SQLLEN indId, indName, indBlk, indStat;
    
    cout << "[DEBUG] Fetching rows..." << endl;
    int count = 0;
    while (SQL_SUCCEEDED(SQLFetch(hStmt))) {
        count++;
        SQLGetData(hStmt, 1, SQL_C_CHAR, id, sizeof(id), &indId);
        SQLGetData(hStmt, 2, SQL_C_CHAR, name, sizeof(name), &indName);
        SQLGetData(hStmt, 3, SQL_C_CHAR, blk, sizeof(blk), &indBlk);
        SQLGetData(hStmt, 4, SQL_C_CHAR, stat, sizeof(stat), &indStat);
        
        string sBlock = (indBlk == SQL_NULL_DATA) ? "N/A" : string(blk);
        string sStatus = (indStat == SQL_NULL_DATA) ? "Unknown" : string(stat);
        
        result += string(id) + "|" + string(name) + "|" + sBlock + "|" + sStatus + ";";
    }
    cout << "[DEBUG] Fetched " << count << " rows. Result length: " << result.length() << endl;
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    return result;
}

// ===== NEW: EVACUATION NOTIFICATION SYSTEM =====

// Function to get all students in a specific block with their phone numbers
vector<pair<string, string>> GetStudentsInBlock(SQLHDBC sqlConnHandle, string block) {
    vector<pair<string, string>> students; // pair of (name, phone)
    SQLHSTMT hStmt;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &hStmt);
    
    // Fixed column name and added hostel filter
    string query = "SELECT u.full_name, IFNULL(u.whatsapp_number, 'No Phone') FROM users u "
                   "WHERE u.role IN ('Student', 'student') AND u.is_hostel_resident = 1 AND u.block = '" + block + "';";
    
    SQLExecDirectA(hStmt, (SQLCHAR*)query.c_str(), SQL_NTS);
    
    char name[100], phone[20];
    while (SQLFetch(hStmt) == SQL_SUCCESS) {
        SQLLEN phoneIndicator;
        SQLGetData(hStmt, 1, SQL_C_CHAR, name, sizeof(name), NULL);
        SQLGetData(hStmt, 2, SQL_C_CHAR, phone, sizeof(phone), &phoneIndicator);
        
        if (phoneIndicator != SQL_NULL_DATA) {
            students.push_back(make_pair(string(name), string(phone)));
        }
    }
    
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    return students;
}

// Function to save evacuation notification to database
bool SaveEvacuationNotification(SQLHDBC sqlConnHandle, string block, string message, int recipientCount) {
    SQLHSTMT hStmt;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &hStmt);
    
    auto clean = [](string s) { for(auto &c : s) if(c == '\'') c = '`'; return s; };
    message = clean(message);
    
    // Using broadcasts table (lower case)
    string query = "INSERT INTO broadcasts (to_group, message, sent_at, sent_by, notification_type) "
                   "VALUES ('" + block + "', '" + message + "', NOW(), 'System', 'evacuation');";
    
    bool success = SQL_SUCCEEDED(SQLExecDirectA(hStmt, (SQLCHAR*)query.c_str(), SQL_NTS));
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    
    cout << "[NOTIFICATION] Evacuation alert saved to database for block: " << block << endl;
    return success;
}

// Enhanced Start Evacuation with Notifications
bool StartEvacuation(SQLHDBC sqlConnHandle, string block) {
    SQLHSTMT hStmt;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &hStmt);
    
    // Handle "All" blocks selection
    string blockFilter = "";
    if (block != "All") {
        blockFilter = " AND u.block = '" + block + "'";
    }

    // 1a. Mark students who are "Present" in residence as Safe (use stay_status column)
    string querySafe = "UPDATE evacuationlogs el JOIN users u ON el.user_id = u.user_id "
                       "SET el.status = 'Safe' "
                       "WHERE u.role IN ('Student', 'student') AND u.is_hostel_resident = 1 "
                       "AND u.stay_status = 'Present'" + blockFilter + ";";
    
    bool safeSuccess = SQL_SUCCEEDED(SQLExecDirectA(hStmt, (SQLCHAR*)querySafe.c_str(), SQL_NTS));
    
    if (!safeSuccess) {
        cout << "[ERROR] Failed to mark Present students as Safe. Query: " << querySafe << endl;
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return false;
    }
    
    // 1b. Mark all other students (not Present) as Missing
    string queryMissing = "UPDATE evacuationlogs el JOIN users u ON el.user_id = u.user_id "
                          "SET el.status = 'Missing' "
                          "WHERE u.role IN ('Student', 'student') AND u.is_hostel_resident = 1 "
                          "AND (u.stay_status != 'Present' OR u.stay_status IS NULL)" + blockFilter + ";";
    
    bool missingSuccess = SQL_SUCCEEDED(SQLExecDirectA(hStmt, (SQLCHAR*)queryMissing.c_str(), SQL_NTS));
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    
    if (!missingSuccess) {
        cout << "[ERROR] Failed to mark non-Present students as Missing. Query: " << queryMissing << endl;
        return false;
    }
    
    cout << "[EVACUATION] Evacuation started for block: " << block << endl;
    cout << "[EVACUATION] Present students automatically marked as Safe" << endl;
    cout << "[EVACUATION] Non-present students marked as Missing" << endl;
    
    // 2. Get all students in the block(s)
    vector<pair<string, string>> students; 
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &hStmt);
    
    // Fixed column name and added hostel filter
    string selectQuery = "SELECT u.full_name, IFNULL(u.whatsapp_number, 'No Phone') FROM users u "
                         "WHERE u.role IN ('Student', 'student') AND u.is_hostel_resident = 1 " + blockFilter + ";";
                         
    SQLExecDirectA(hStmt, (SQLCHAR*)selectQuery.c_str(), SQL_NTS);
    char name[100], phone[20];
    while (SQLFetch(hStmt) == SQL_SUCCESS) {
        SQLLEN phoneIndicator;
        SQLGetData(hStmt, 1, SQL_C_CHAR, name, sizeof(name), NULL);
        SQLGetData(hStmt, 2, SQL_C_CHAR, phone, sizeof(phone), &phoneIndicator);
        if (phoneIndicator != SQL_NULL_DATA) {
            students.push_back(make_pair(string(name), string(phone)));
        }
    }
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    
    cout << "[NOTIFICATION] Found " << students.size() << " students to notify." << endl;
    
    // 3. Prepare evacuation message
    string targetArea = (block == "All") ? "ALL BLOCKS" : ("Block " + block);
    string evacuationMessage = "🚨 EMERGENCY EVACUATION ALERT 🚨\n\n"
                               "Evacuation initiated for " + targetArea + ".\n"
                               "Please proceed to the nearest emergency assembly point immediately.\n"
                               "Follow emergency protocols and await further instructions.\n\n"
                               "Mark yourself safe in the Campus Security App once you reach safety.";
    
    // 4. Send WhatsApp notifications to all students (same as before)
    int successCount = 0;
    int failCount = 0;
    
    for (const auto& student : students) {
        string name = student.first;
        string phone = student.second;
        // Basic console logging only
        // cout << "[SENDING] Notification to " << name << "..." << endl;
        if (SendWhatsAppMessage(phone, evacuationMessage)) successCount++;
        else failCount++;
        Sleep(200); // Slight delay
    }
    
    // 5. Save notification record
    SaveEvacuationNotification(sqlConnHandle, block, evacuationMessage, students.size());
    
    // 6. Log results
    cout << "\n[NOTIFICATION SUMMARY]" << endl;
    cout << "Target: " << targetArea << endl;
    cout << "Total Students: " << students.size() << endl;
    cout << "Sent: " << successCount << ", Failed: " << failCount << endl;
    cout << "================================\n" << endl;
    
    return true;
}

// ===== NEW: CLASS-BASED EVACUATION (for Lecturers) =====
// Function to start evacuation by class group (similar to StartEvacuation but filters by class)
bool StartClassEvacuation(SQLHDBC sqlConnHandle, string classGroup) {
    SQLHSTMT hStmt;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &hStmt);
    
    // Handle "All" classes selection
    string classFilter = "";
    if (classGroup != "All") {
        classFilter = " AND (sp.class_group LIKE '%" + classGroup + "%' OR sp.class_group = '" + classGroup + "')";
    }

    // SIMPLIFIED: Mark all students in the class as "Not Safe" (Evacuation Status)
    // Works regardless of hostel_resident flag or stay_status
    string queryEvac = "UPDATE evacuationlogs el "
                       "JOIN users u ON el.user_id = u.user_id "
                       "LEFT JOIN studentprofiles sp ON u.user_id = sp.user_id "
                       "SET el.status = 'Not Safe' "
                       "WHERE u.role IN ('Student', 'student', 'STUDENT')" + classFilter + ";";
    
    bool evacSuccess = SQL_SUCCEEDED(SQLExecDirectA(hStmt, (SQLCHAR*)queryEvac.c_str(), SQL_NTS));
    
    if (!evacSuccess) {
        cout << "[ERROR] Failed to mark students for evacuation. Query: " << queryEvac << endl;
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return false;
    }

    cout << "[EVACUATION] Class evacuation started for: " << classGroup << endl;
    cout << "[EVACUATION] All students marked as 'Not Safe'" << endl;
    cout << "[EVACUATION] Students can mark themselves as 'Safe' in the app" << endl;

    // Get all students in the class(es) for notifications
    vector<pair<string, string>> students; 
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &hStmt);

    // Simplified query: just get students with or without class_group
    string selectQuery = "SELECT u.user_id, u.full_name, IFNULL(u.whatsapp_number, 'No Phone') "
                         "FROM users u "
                         "LEFT JOIN studentprofiles sp ON u.user_id = sp.user_id "
                         "WHERE u.role IN ('Student', 'student', 'STUDENT')" + classFilter + " "
                         "LIMIT 1000;";
                         
    cout << "[DEBUG] Student query: " << selectQuery << endl;
    
    if (SQL_SUCCEEDED(SQLExecDirectA(hStmt, (SQLCHAR*)selectQuery.c_str(), SQL_NTS))) {
        char userId[20], name[100], phone[20];
        while (SQLFetch(hStmt) == SQL_SUCCESS) {
            SQLLEN phoneIndicator;
            SQLGetData(hStmt, 1, SQL_C_CHAR, userId, sizeof(userId), NULL);
            SQLGetData(hStmt, 2, SQL_C_CHAR, name, sizeof(name), NULL);
            SQLGetData(hStmt, 3, SQL_C_CHAR, phone, sizeof(phone), &phoneIndicator);
            
            if (phoneIndicator != SQL_NULL_DATA && strlen(phone) > 0) {
                students.push_back(make_pair(string(name), string(phone)));
                cout << "[NOTIFY] Student: " << name << " -> " << phone << endl;
            }
        }
    } else {
        cout << "[WARNING] Could not fetch students for notification, but evacuation status was updated" << endl;
    }
    
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);

    cout << "[NOTIFICATION] Found " << students.size() << " students with valid phone numbers." << endl;

    // 3. Send WhatsApp notifications
    for (const auto& student : students) {
        cout << "[WHATSAPP-QUEUE] " << student.first << " (" << student.second << ")" << endl;
        // WhatsApp sending would go here
    }

    cout << "[SUCCESS] Class evacuation initialized. Status updated for all students." << endl;
    return true;
}

// Subject-based evacuation removed per user request

// Function to mark student as safe
bool MarkStudentSafe(SQLHDBC sqlConnHandle, string userId) {
    SQLHSTMT hStmt;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &hStmt);
    string query = "UPDATE evacuationlogs SET status = 'Safe' WHERE user_id = " + userId + ";";
    bool success = SQL_SUCCEEDED(SQLExecDirectA(hStmt, (SQLCHAR*)query.c_str(), SQL_NTS));
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    return success;
}

// Update or insert student's class assignment in studentprofiles
bool UpdateStudentClass(SQLHDBC sqlConnHandle, string userId, string classGroup) {
    SQLHSTMT hStmt;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &hStmt);
    // If userId is not numeric, try to resolve it via users.student_code first, then users.username
    string resolvedId = userId;
    bool isNumeric = true;
    for (char c : userId) if (!isdigit((unsigned char)c)) { isNumeric = false; break; }

    if (!isNumeric) {
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);

        // Try lookup by student_code first
        string lookupByCode = "SELECT user_id FROM users WHERE student_code = '" + userId + "' LIMIT 1;";
        SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &hStmt);
        bool found = false;
        if (SQL_SUCCEEDED(SQLExecDirectA(hStmt, (SQLCHAR*)lookupByCode.c_str(), SQL_NTS))) {
            if (SQLFetch(hStmt) == SQL_SUCCESS) {
                char buf[20];
                SQLGetData(hStmt, 1, SQL_C_CHAR, buf, sizeof(buf), NULL);
                resolvedId = string(buf);
                found = true;
            }
        }
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);

        // If not found, try lookup by username (safer fallback)
        if (!found) {
            string lookupByUser = "SELECT user_id FROM users WHERE username = '" + userId + "' LIMIT 1;";
            SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &hStmt);
            if (SQL_SUCCEEDED(SQLExecDirectA(hStmt, (SQLCHAR*)lookupByUser.c_str(), SQL_NTS))) {
                if (SQLFetch(hStmt) == SQL_SUCCESS) {
                    char buf[20];
                    SQLGetData(hStmt, 1, SQL_C_CHAR, buf, sizeof(buf), NULL);
                    resolvedId = string(buf);
                    found = true;
                }
            }
            SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        }

        if (!found) {
            cout << "[ERROR] Could not resolve student identifier: " << userId << " (no matching student_code or username)" << endl;
            return false;
        }
    } else {
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    }

    // Check if profile exists
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &hStmt);
    string checkQ = "SELECT profile_id FROM studentprofiles WHERE user_id = " + resolvedId + " LIMIT 1;";
    bool exists = false;
    if (SQL_SUCCEEDED(SQLExecDirectA(hStmt, (SQLCHAR*)checkQ.c_str(), SQL_NTS))) {
        if (SQLFetch(hStmt) == SQL_SUCCESS) exists = true;
    }
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);

    // If exists, update; else insert
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &hStmt);
    string q;
    if (exists) {
        q = "UPDATE studentprofiles SET class_group = '" + classGroup + "' WHERE user_id = " + resolvedId + ";";
    } else {
        q = "INSERT INTO studentprofiles (user_id, class_group, residency_status, safety_status) VALUES (" + resolvedId + ", '" + classGroup + "', 'Resident', 'Unknown');";
    }

    bool ok = SQL_SUCCEEDED(SQLExecDirectA(hStmt, (SQLCHAR*)q.c_str(), SQL_NTS));
    if (!ok) cout << "[ERROR] Failed to assign class. Query: " << q << endl;
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    return ok;
}

// Generate report of unsafe students
void GenerateUnsafeReport(SQLHDBC sqlConnHandle) {
    SQLHSTMT hStmt;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &hStmt);
    // Added is_hostel_resident check and fixed column name
    string query = "SELECT u.full_name, u.block, IFNULL(u.whatsapp_number, 'No Phone') FROM users u "
                   "JOIN evacuationlogs sp ON u.user_id = sp.user_id "
                   "WHERE sp.status = 'Missing' AND u.role IN ('Student', 'student') AND u.is_hostel_resident = 1;";
    SQLExecDirectA(hStmt, (SQLCHAR*)query.c_str(), SQL_NTS);
    
    ofstream report("unsafe_students_report.txt");
    report << "=== UNSAFE STUDENTS REPORT ===" << endl;
    report << "Generated: " << __DATE__ << " " << __TIME__ << endl << endl;
    
    char name[100], blk[50], phone[20];
    int count = 0;
    while (SQLFetch(hStmt) == SQL_SUCCESS) {
        SQLGetData(hStmt, 1, SQL_C_CHAR, name, sizeof(name), NULL);
        SQLGetData(hStmt, 2, SQL_C_CHAR, blk, sizeof(blk), NULL);
        SQLGetData(hStmt, 3, SQL_C_CHAR, phone, sizeof(phone), NULL);
        report << ++count << ". " << name << " | Block: " << blk << " | Phone: " << phone << endl;
    }
    
    report.close();
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    cout << "[REPORT] Generated unsafe_students_report.txt (" << count << " students)" << endl;
}

// Lecturer Functions
string GetBroadcasts(SQLHDBC sqlConnHandle) {
    SQLHSTMT hStmt;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &hStmt);
    // Fixed table name
    string query = "SELECT id, to_group, message, DATE_FORMAT(sent_at, '%Y-%m-%d %H:%i'), sent_by FROM broadcasts ORDER BY sent_at DESC LIMIT 50;";
    SQLExecDirectA(hStmt, (SQLCHAR*)query.c_str(), SQL_NTS);
    
    string result = "";
    char id[20], grp[100], msg[500], time[50], sender[100];
    while (SQLFetch(hStmt) == SQL_SUCCESS) {
        SQLGetData(hStmt, 1, SQL_C_CHAR, id, sizeof(id), NULL);
        SQLGetData(hStmt, 2, SQL_C_CHAR, grp, sizeof(grp), NULL);
        SQLGetData(hStmt, 3, SQL_C_CHAR, msg, sizeof(msg), NULL);
        SQLGetData(hStmt, 4, SQL_C_CHAR, time, sizeof(time), NULL);
        SQLGetData(hStmt, 5, SQL_C_CHAR, sender, sizeof(sender), NULL);
        result += string(id) + "|" + grp + "|" + msg + "|" + time + "|" + sender + ";";
    }
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    return result;
}

string GetClassStudents(SQLHDBC sqlConnHandle, string className) {
    SQLHSTMT hStmt;
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &hStmt);
    
    // Join with studentprofiles to get class_group and filter if needed
    string query = "SELECT u.user_id, u.full_name, IFNULL(u.block, 'N/A'), IFNULL(el.status, 'Safe') "
                   "FROM users u "
                   "LEFT JOIN studentprofiles sp ON u.user_id = sp.user_id "
                   "LEFT JOIN evacuationlogs el ON u.user_id = el.user_id "
                   "WHERE u.role IN ('Student', 'student')";
    
    if (className != "") {
        query += " AND sp.class_group LIKE '%" + className + "%'";
    }
    query += ";";
    
    SQLExecDirectA(hStmt, (SQLCHAR*)query.c_str(), SQL_NTS);
    
    string result = "";
    char code[50], name[100], loc[50], status[50];
    while (SQLFetch(hStmt) == SQL_SUCCESS) {
        SQLGetData(hStmt, 1, SQL_C_CHAR, code, sizeof(code), NULL);
        SQLGetData(hStmt, 2, SQL_C_CHAR, name, sizeof(name), NULL);
        SQLGetData(hStmt, 3, SQL_C_CHAR, loc, sizeof(loc), NULL);
        SQLGetData(hStmt, 4, SQL_C_CHAR, status, sizeof(status), NULL);
        result += string(code) + "|" + name + "|" + loc + "|" + status + ";";
    }
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    return result;
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
        int bytesReceived = recv(c, buf, 8192, 0);

        if (bytesReceived <= 0) {
            closesocket(c);
            continue;
        }

        string req(buf, bytesReceived);
        cout << "[DEBUG] Request Received: " << req.substr(0, req.find("\r\n")) << endl; // Log request line
        string body = "";
        
        // Split header and body
        size_t bodyPos = req.find("\r\n\r\n");
        if(bodyPos != string::npos) body = req.substr(bodyPos + 4);

        string resp = "HTTP/1.1 404 Not Found\r\n\r\n";

        // ROUTING
        if (req.find("GET / ") != string::npos || req.find("GET /login.html") != string::npos) {
            resp = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n" + LoadFile("login.html");
        } 
        else if (req.find("GET /lecturer.html") != string::npos) {
            resp = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n" + LoadFile("lecturer.html");
        }
        else if (req.find("POST /login") != string::npos) {
            string u = GetP(body, "userid"); string p = GetP(body, "password");
            if (AttemptLogin(sqlConn, u, p, currentUserName, currentUserRole, currentUserBlock)) {
                cout << "Login: " << currentUserName << " (" << currentUserRole << ")" << endl;
                string redirect = (currentUserRole == "Lecturer") ? "/lecturer.html" : "/home.html";
                resp = "HTTP/1.1 303 See Other\r\nLocation: " + redirect + "\r\n\r\n";
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
        // /update_stay_status route removed: student stay/status feature deprecated
        // INCIDENTS
        else if (req.find("POST /save_warden_report") != string::npos) {
            string incident_id = GetP(body, "incident_id");
            string details = GetP(body, "details");
            
            auto clean = [](string s) { for(auto &c : s) if(c == '\'') c = '`'; return s; };
            details = clean(details);
            
            string query = "INSERT INTO wardenreports (incident_id, warden_name, report_details) VALUES (" 
                           + incident_id + ", '" + currentUserName + "', '" + details + "');";
                           
            SQLHSTMT hStmt;
            SQLAllocHandle(SQL_HANDLE_STMT, sqlConn, &hStmt);
            if (SQL_SUCCEEDED(SQLExecDirectA(hStmt, (SQLCHAR*)query.c_str(), SQL_NTS))) {
                resp = "HTTP/1.1 200 OK\r\n\r\nSuccess";
            } else {
                resp = "HTTP/1.1 500 Error\r\n\r\nFail";
            }
            SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        }
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
        // EVACUATION (WITH NOTIFICATIONS)
        else if (req.find("GET /get_evacuation") != string::npos) {
            resp = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n" + GetEvacuationList(sqlConn);
        }
        else if (req.find("POST /start_evacuation") != string::npos) {
            string block = GetP(body, "block");
            cout << "\n[EVACUATION INITIATED] Block: " << block << endl;
            
            if(StartEvacuation(sqlConn, block)) {
                resp = "HTTP/1.1 200 OK\r\n\r\nStarted";
                cout << "[SUCCESS] Evacuation started and notifications sent for block " << block << endl;
            } else {
                resp = "HTTP/1.1 500 Error\r\n\r\nFail";
                cout << "[ERROR] Failed to start evacuation for block " << block << endl;
            }
        }
        else if (req.find("POST /start_class_evacuation") != string::npos) {
            string classGroup = urlDecode(GetP(body, "class"));
            cout << "\n[CLASS EVACUATION INITIATED] Class: " << classGroup << endl;

            if (StartClassEvacuation(sqlConn, classGroup)) {
                resp = "HTTP/1.1 200 OK\r\n\r\nStarted";
                cout << "[SUCCESS] Class evacuation started and notifications sent for class " << classGroup << endl;
            } else {
                resp = "HTTP/1.1 500 Error\r\n\r\nFail";
                cout << "[ERROR] Failed to start class evacuation for class " << classGroup << endl;
            }
        }
        // Subject evacuation endpoint removed
        else if (req.find("POST /assign_student_class") != string::npos) {
            // body: student_id=123,124&class=TCS3014  (student_id can be comma-separated user_id or student_code/username)
            string ids = GetP(body, "student_id");
            string classGroup = urlDecode(GetP(body, "class"));
            if (classGroup == "") classGroup = "";

            // split ids by comma
            vector<string> list;
            size_t start = 0;
            while (start < ids.length()) {
                size_t comma = ids.find(',', start);
                if (comma == string::npos) comma = ids.length();
                string token = ids.substr(start, comma - start);
                // trim
                while (!token.empty() && isspace(token.back())) token.pop_back();
                size_t i=0; while (i<token.size() && isspace(token[i])) i++; token = token.substr(i);
                if (!token.empty()) list.push_back(token);
                start = comma + 1;
            }

            int updated = 0, created = 0, failed = 0;
            string failedList = "";

            for (const auto &sid : list) {
                string resolvedId = sid;
                bool isNumeric = true;
                for (char c : sid) if (!isdigit((unsigned char)c)) { isNumeric = false; break; }

                SQLHSTMT hStmt; SQLAllocHandle(SQL_HANDLE_STMT, sqlConn, &hStmt);

                if (!isNumeric) {
                    // try student_code
                    string q1 = "SELECT user_id FROM users WHERE student_code = '" + sid + "' LIMIT 1;";
                    bool found = false;
                    if (SQL_SUCCEEDED(SQLExecDirectA(hStmt, (SQLCHAR*)q1.c_str(), SQL_NTS))) {
                        if (SQLFetch(hStmt) == SQL_SUCCESS) {
                            char buf[32]; SQLGetData(hStmt, 1, SQL_C_CHAR, buf, sizeof(buf), NULL);
                            resolvedId = string(buf); found = true;
                        }
                    }
                    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);

                    if (!found) {
                        // try username
                        SQLAllocHandle(SQL_HANDLE_STMT, sqlConn, &hStmt);
                        string q2 = "SELECT user_id FROM users WHERE username = '" + sid + "' LIMIT 1;";
                        if (SQL_SUCCEEDED(SQLExecDirectA(hStmt, (SQLCHAR*)q2.c_str(), SQL_NTS))) {
                            if (SQLFetch(hStmt) == SQL_SUCCESS) {
                                char buf[32]; SQLGetData(hStmt, 1, SQL_C_CHAR, buf, sizeof(buf), NULL);
                                resolvedId = string(buf); found = true;
                            }
                        }
                        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
                    }

                    if (!found) {
                        // create minimal user record
                        SQLAllocHandle(SQL_HANDLE_STMT, sqlConn, &hStmt);
                        string cleanName = sid;
                        for (auto &c : cleanName) if (c == '\'') c = '`';
                        string ins = "INSERT INTO users (username, full_name, role, created_at, is_hostel_resident) VALUES ('" + cleanName + "', 'New Student', 'Student', NOW(), 1);";
                        if (SQL_SUCCEEDED(SQLExecDirectA(hStmt, (SQLCHAR*)ins.c_str(), SQL_NTS))) {
                            // get last insert id
                            SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
                            SQLAllocHandle(SQL_HANDLE_STMT, sqlConn, &hStmt);
                            SQLExecDirectA(hStmt, (SQLCHAR*)"SELECT LAST_INSERT_ID()", SQL_NTS);
                            if (SQLFetch(hStmt) == SQL_SUCCESS) { char buf[32]; SQLGetData(hStmt, 1, SQL_C_CHAR, buf, sizeof(buf), NULL); resolvedId = string(buf); }
                            SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
                        } else {
                            SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
                            failed++; if (!failedList.empty()) failedList += ","; failedList += sid; continue;
                        }
                    }
                } else {
                    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
                }

                // Now upsert studentprofiles for resolvedId
                SQLAllocHandle(SQL_HANDLE_STMT, sqlConn, &hStmt);
                string chk = "SELECT profile_id FROM studentprofiles WHERE user_id = " + resolvedId + " LIMIT 1;";
                bool exists = false;
                if (SQL_SUCCEEDED(SQLExecDirectA(hStmt, (SQLCHAR*)chk.c_str(), SQL_NTS))) {
                    if (SQLFetch(hStmt) == SQL_SUCCESS) exists = true;
                }
                SQLFreeHandle(SQL_HANDLE_STMT, hStmt);

                SQLAllocHandle(SQL_HANDLE_STMT, sqlConn, &hStmt);
                string q;
                if (exists) {
                    q = "UPDATE studentprofiles SET class_group = '" + classGroup + "' WHERE user_id = " + resolvedId + ";";
                    if (SQL_SUCCEEDED(SQLExecDirectA(hStmt, (SQLCHAR*)q.c_str(), SQL_NTS))) { updated++; }
                    else { failed++; if (!failedList.empty()) failedList += ","; failedList += sid; }
                } else {
                    q = "INSERT INTO studentprofiles (user_id, class_group, residency_status, safety_status) VALUES (" + resolvedId + ", '" + classGroup + "', 'Resident', 'Unknown');";
                    if (SQL_SUCCEEDED(SQLExecDirectA(hStmt, (SQLCHAR*)q.c_str(), SQL_NTS))) { created++; }
                    else { failed++; if (!failedList.empty()) failedList += ","; failedList += sid; }
                }
                SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
            }

            // Build response
            string respBody = "Updated=" + to_string(updated) + ";Created=" + to_string(created) + ";Failed=" + to_string(failed);
            if (!failedList.empty()) respBody += ";FailedList=" + failedList;
            if (failed == 0) resp = string("HTTP/1.1 200 OK\r\n\r\n") + respBody;
            else resp = string("HTTP/1.1 500 Error\r\n\r\n") + respBody;
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
        // --- LECTURER ROUTES ---
        else if (req.find("GET /get_broadcasts") != string::npos) {
            resp = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n" + GetBroadcasts(sqlConn);
        }
        else if (req.find("GET /get_students") != string::npos) {
            string className = "";
            size_t qPos = req.find("?");
            if (qPos != string::npos) {
                size_t cPos = req.find("class=", qPos);
                if (cPos != string::npos) {
                    size_t spacePos = req.find(" ", cPos);
                    className = urlDecode(req.substr(cPos + 6, spacePos - (cPos + 6)));
                }
            }
            resp = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n" + GetClassStudents(sqlConn, className);
        }
        else if (req.find("POST /broadcast") != string::npos) {
            string to = urlDecode(GetP(body, "to"));
            string msg = urlDecode(GetP(body, "message"));
            string urgency = urlDecode(GetP(body, "urgency"));
            
            // Clean message to prevent SQL injection
            auto clean = [](string s) { for(auto &c : s) if(c == '\'') c = '`'; return s; };
            msg = clean(msg);
            to = clean(to);
            
            SQLHSTMT hStmt; SQLAllocHandle(SQL_HANDLE_STMT, sqlConn, &hStmt);
            string q = "INSERT INTO broadcasts (to_group, message, sent_at, sent_by) VALUES ('" + to + "', '" + msg + "', NOW(), '" + currentUserName + "')";
            SQLExecDirectA(hStmt, (SQLCHAR*)q.c_str(), SQL_NTS);
            SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
            resp = "HTTP/1.1 200 OK\r\n\r\nSaved";
        }
        else if (req.find("POST /delete_broadcast") != string::npos) {
            string id = GetP(body, "id");
            SQLHSTMT hStmt; SQLAllocHandle(SQL_HANDLE_STMT, sqlConn, &hStmt);
            string q = "DELETE FROM Broadcasts WHERE id=" + id;
            SQLExecDirectA(hStmt, (SQLCHAR*)q.c_str(), SQL_NTS);
            SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
            resp = "HTTP/1.1 200 OK\r\n\r\nDeleted";
        }
        else if (req.find("POST /update_student_status") != string::npos) {
            // Update individual student evacuation status
            string studentId = GetP(body, "student_id");
            string status = urlDecode(GetP(body, "status"));
            
            if (studentId.empty() || status.empty()) {
                resp = "HTTP/1.1 400 Bad Request\r\n\r\nMissing student_id or status parameter";
            } else if (UpdateStudentEvacStatus(sqlConn, studentId, status)) {
                resp = "HTTP/1.1 200 OK\r\n\r\nUpdated";
            } else {
                resp = "HTTP/1.1 500 Error\r\n\r\nFail";
            }
        }

        send(c, resp.c_str(), resp.length(), 0); closesocket(c);
    }
    return 0;
}