#include <iostream>
#include <string>
#include <vector>
#include <windows.h> 
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
using namespace std;
// PASTE YOUR TOKEN HERE FOR TESTING
const string PHONE_NUMBER_ID = "962408263622848";
const string ACCESS_TOKEN = "EAAjQvZASvqfkBQj1aeraJXBZBWWFPrNKymw36tp11iWfyEKTuC28YQ1emGZChRnSD7t7icjqkP7IecFXA7SNQE6jyqYInyAp97tKhUFhLXD73fthpcTbi0PCzJRsxeniAFhQMrZCAYWrJqEvO5bgfywxFgsa4SakacrztZBrzSGUcZAGWvaHaEdteZCcdhIrmdcrZBgy4pAKADWFVAtCsI4tOKx22xf9XZCmFxifrIQfvLWprBOQnrhAgvEoLWnClGj4eM9PH0iF6otVuLtaYcc9Ug7ga"; // User to replace this if needed
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