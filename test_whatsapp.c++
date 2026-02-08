#include <iostream>
#include <string>
#include <vector>
#include <windows.h>
#include <winhttp.h>

#pragma comment(lib, "winhttp.lib")

using namespace std;

// WhatsApp Config - COPY FROM login.c++
const string PHONE_NUMBER_ID = "962408263622848";
const string ACCESS_TOKEN = "EAAjQvZASvqfkBQrQvBZCzhZAOJs1FOk7p8yrnU5nNrLj6FEwS9l4ilZBid740ITgI9R88quIBUpXKYkiAnwuYrjN18TXUiR6D8g8fTjiNBkBNJBnmeritnB8nt8I0AsOyTYohqexJPTf0DXrOvvNLad5ZCP3IwMYCA1nyzAi1qzWGEuCfDYWiy5PF8zNwTNKufvd1LZBSPqrMo24SQ9lmBqHsddxVBZCjb0gmkZCLRZAZBPfLgZB6RAUR8SXlXRZAZBZB8BBO8N4df0EpIkNyzMzuGYsk4G26W";

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

bool TestWhatsAppMessage(const string& recipient, const string& messageText) {
    // 0. Format Phone Number
    string cleanPhone = "";
    for (char c : recipient) {
        if (isdigit(c)) cleanPhone += c;
    }
    if (cleanPhone.length() > 2 && cleanPhone[0] == '0') {
         cleanPhone = "60" + cleanPhone.substr(1);
    }

    cout << "========================================" << endl;
    cout << "WHATSAPP API TEST" << endl;
    cout << "========================================" << endl;
    cout << "Phone Number ID: " << PHONE_NUMBER_ID << endl;
    cout << "Access Token: " << ACCESS_TOKEN.substr(0, 20) << "..." << endl;
    cout << "Recipient (cleaned): " << cleanPhone << endl;
    cout << "Message: " << messageText << endl;
    cout << "========================================" << endl;

    // 1. Prepare Request Data
    wstring host = L"graph.facebook.com";
    wstring path = L"/v20.0/" + wstring(PHONE_NUMBER_ID.begin(), PHONE_NUMBER_ID.end()) + L"/messages";

    // Initialize WinHTTP
    HINTERNET hSession = WinHttpOpen(L"CampusSecurity/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) {
        cout << "[ERROR] Failed to initialize WinHTTP session" << endl;
        return false;
    }

    HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", path.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);

    // 2. Setup JSON Payload and Authorization
    string escapedMsg = JsonEscape(messageText);
    string json = "{\"messaging_product\":\"whatsapp\",\"to\":\"" + cleanPhone + "\",\"type\":\"text\",\"text\":{\"body\":\"" + escapedMsg + "\"}}";
    string auth = "Authorization: Bearer " + ACCESS_TOKEN;

    cout << "\n[REQUEST]" << endl;
    cout << "URL: https://" << string(host.begin(), host.end()) << string(path.begin(), path.end()) << endl;
    cout << "JSON Payload: " << json << endl;
    cout << "\n[SENDING REQUEST...]" << endl;

    // 3. Add Headers
    WinHttpAddRequestHeaders(hRequest, wstring(auth.begin(), auth.end()).c_str(), (DWORD)-1L, WINHTTP_ADDREQ_FLAG_ADD);
    WinHttpAddRequestHeaders(hRequest, L"Content-Type: application/json", (DWORD)-1L, WINHTTP_ADDREQ_FLAG_ADD);

    // 4. Send the Request
    BOOL result = WinHttpSendRequest(
        hRequest, 
        WINHTTP_NO_ADDITIONAL_HEADERS, 0, 
        (LPVOID)json.c_str(), (DWORD)json.size(), 
        (DWORD)json.size(), 0
    );

    if (result) {
        result = WinHttpReceiveResponse(hRequest, NULL);
    }

    // 5. Check Status Code
    if (result) {
        DWORD dwStatusCode = 0;
        DWORD dwSize = sizeof(dwStatusCode);
        WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &dwStatusCode, &dwSize, WINHTTP_NO_HEADER_INDEX);
        
        cout << "\n[RESPONSE]" << endl;
        cout << "HTTP Status Code: " << dwStatusCode << endl;
        
        // Read response body
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
        
        cout << "Response Body: " << apiResponse << endl;
        cout << "\n========================================" << endl;
        
        if (dwStatusCode == 200 || dwStatusCode == 201) {
            cout << "✅ SUCCESS! Message sent successfully!" << endl;
        } else if (dwStatusCode == 401) {
            cout << "❌ FAILED: 401 Unauthorized - Your access token has EXPIRED!" << endl;
            cout << "💡 Solution: Generate a new token from Meta Business Suite" << endl;
        } else if (dwStatusCode == 400) {
            cout << "❌ FAILED: 400 Bad Request" << endl;
            cout << "💡 Possible causes:" << endl;
            cout << "   - Phone number not registered in Meta Sandbox" << endl;
            cout << "   - Invalid phone number format" << endl;
            cout << "   - Invalid JSON payload" << endl;
        } else {
            cout << "❌ FAILED: Unexpected status code " << dwStatusCode << endl;
        }
        cout << "========================================" << endl;
        
        result = (dwStatusCode == 200 || dwStatusCode == 201);
    } else {
        cout << "\n[ERROR] Could not connect to Meta Servers" << endl;
        cout << "Error Code: " << GetLastError() << endl;
    }

    // 6. Cleanup
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return result;
}

int main() {
    cout << "WhatsApp API Connectivity Test\n" << endl;
    
    // Test with a sample phone number
    string testPhone;
    cout << "Enter test phone number (e.g., 60123456789 or 0123456789): ";
    getline(cin, testPhone);
    
    string testMessage = "🧪 TEST MESSAGE\n\nThis is a test message from the Campus Security System.\nIf you receive this, WhatsApp integration is working!";
    
    bool success = TestWhatsAppMessage(testPhone, testMessage);
    
    if (success) {
        cout << "\n✅ Test completed successfully!" << endl;
    } else {
        cout << "\n❌ Test failed. Please check the error messages above." << endl;
    }
    
    cout << "\nPress Enter to exit...";
    cin.get();
    return 0;
}
