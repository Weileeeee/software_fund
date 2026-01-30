#include <iostream>
#include <string>
#include <vector>
#include <windows.h> 
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
using namespace std;
// PASTE YOUR TOKEN HERE FOR TESTING
const string PHONE_NUMBER_ID = "962408263622848";
const string ACCESS_TOKEN = "EAAjQvZASvqfkBQgLGPd27m9W0Il2byEYVh8kOAPsMQYKoFAoeld1grzumRW1HH2TPQciXLUm7wjoVQZCTKIjM8vYPkiAbk1PCk6YyhwDObPiZANkpas11n39RTmKqZCcmPZCWUZA5HrIOlc6eZCbDdsE0DZC9EqIiO7SJOUEDeZCAs0UdZAKDjxJvOuBvyWV89psM1jBLuPWmD5XmVk8tVum6x73e1bHb8ikSWIF0e0O4isVlMV3qVHb0ZBxbZA3ZBjIcrB3LT9WZAQGxXod1moWP7bBEIbMPPRQZDZD"; // User to replace this if needed
const string TEST_PHONE_NUMBER = "601110867951"; // Replace with your own number
string JsonEscape(const string& s) {
    // ... (same helper) ...
}
void TestWhatsApp(string recipient, string messageText) {
    // ... (logic from login.c++) ...
    // Added detailed logging
}
int main() {
    cout << "Testing WhatsApp API..." << endl;
    TestWhatsApp(TEST_PHONE_NUMBER, "Hello from Debugger!");
    return 0;
}